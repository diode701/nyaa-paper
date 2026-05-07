package com.nyar;

import com.nyar.config.Playlist;
import com.nyar.config.PlaylistManager;
import com.nyar.config.WallpaperConfig;
import com.nyar.wallpaper.PlaylistRunner;
import javafx.animation.FadeTransition;
import javafx.animation.PauseTransition;
import javafx.animation.TranslateTransition;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.CacheHint;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.util.Duration;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.List;
import java.util.Map;
import java.util.concurrent.*;

public class MiniPlaylistPanel {

    private static final int PANEL_WIDTH = 300;
    private static final int STRIP_WIDTH = 10;

    private final PlaylistManager playlistManager = PlaylistManager.getInstance();
    private final PlaylistRunner runner = PlaylistRunner.getInstance();

    private String currentMonitor = "";
    private final VBox entryList;
    private final Label headerLabel;
    private final StackPane root;
    private final VBox panel;
    private final VBox strip;
    private boolean visible;
    private int selectedEntryIdx = -1;
    private final List<WallpaperConfig> entries = new CopyOnWriteArrayList<>();

    private final PauseTransition hideDelay = new PauseTransition(Duration.millis(400));
    private final ExecutorService imageLoader = Executors.newFixedThreadPool(2, r -> {
        Thread t = new Thread(r, "panel-img");
        t.setDaemon(true);
        return t;
    });
    private final Map<Integer, Image> imageCache = new ConcurrentHashMap<>();

    public MiniPlaylistPanel() {
        // Hover trigger strip
        strip = new VBox();
        strip.setPrefWidth(STRIP_WIDTH);
        strip.setMinWidth(STRIP_WIDTH);
        strip.setMaxWidth(STRIP_WIDTH);
        strip.setStyle("-fx-background-color: transparent;");
        strip.setOnMouseEntered(e -> show());
        VBox.setVgrow(strip, Priority.ALWAYS);

        // Panel content
        headerLabel = new Label("Playlist");
        headerLabel.setTextFill(Color.WHITE);
        headerLabel.setFont(Font.font(14));

        Button closeBtn = new Button("\u2715");
        closeBtn.setStyle("-fx-background-color: transparent; -fx-text-fill: #aaa;");
        closeBtn.setOnAction(e -> hide());

        HBox header = new HBox(10);
        header.setAlignment(Pos.CENTER_LEFT);
        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);
        header.getChildren().addAll(headerLabel, spacer, closeBtn);

        entryList = new VBox(5);
        entryList.setPadding(new Insets(0, 5, 0, 0));

        ScrollPane scroll = new ScrollPane(entryList);
        scroll.setFitToWidth(true);
        scroll.setStyle("-fx-background: transparent; -fx-background-color: transparent;");
        scroll.setHbarPolicy(ScrollPane.ScrollBarPolicy.NEVER);

        Label entryStatus = new Label("");
        entryStatus.setTextFill(Color.LIGHTGRAY);
        entryStatus.setFont(Font.font(11));
        entryStatus.setWrapText(true);

        HBox controls = createControlBar();

        panel = new VBox(8);
        panel.setPrefWidth(PANEL_WIDTH);
        panel.setMinWidth(PANEL_WIDTH);
        panel.setMaxWidth(PANEL_WIDTH);
        panel.setPadding(new Insets(10));
        panel.setStyle("-fx-background-color: #252525ee; -fx-border-color: #3a3a3a; -fx-border-width: 0 0 0 1;");
        panel.getChildren().addAll(header, scroll, entryStatus, controls);
        VBox.setVgrow(scroll, Priority.ALWAYS);

        panel.setTranslateX(PANEL_WIDTH);
        panel.setOnMouseEntered(e -> hideDelay.stop());
        panel.setOnMouseExited(e -> hideDelay.playFromStart());

        hideDelay.setOnFinished(e -> hide());

        root = new StackPane();
        root.setPickOnBounds(false);
        root.getChildren().addAll(strip, panel);
        StackPane.setAlignment(strip, Pos.CENTER_RIGHT);
        StackPane.setAlignment(panel, Pos.CENTER_RIGHT);

        runner.setStatusListener((monitor, status) -> {
            if (monitor.equals(currentMonitor)) {
                Platform.runLater(() -> headerLabel.setText("Playlist \u2014 " + status));
            }
        });
    }

    public StackPane getView() { return root; }

    private HBox createControlBar() {
        HBox bar = new HBox(5);
        bar.setAlignment(Pos.CENTER);

        Button playBtn = controlBtn("\u25B6", "Play");
        Button pauseBtn = controlBtn("\u23F8", "Pause");
        Button stopBtn = controlBtn("\u23F9", "Stop");
        Button resetBtn = controlBtn("\u21BA", "Reset");
        Button prevBtn = controlBtn("\u23EE", "Previous");
        Button nextBtn = controlBtn("\u23ED", "Next");

        playBtn.setOnAction(e -> runner.play(currentMonitor));
        pauseBtn.setOnAction(e -> {
            if (runner.isPaused(currentMonitor)) runner.resume(currentMonitor);
            else runner.pause(currentMonitor);
        });
        stopBtn.setOnAction(e -> runner.stop(currentMonitor));
        resetBtn.setOnAction(e -> runner.reset(currentMonitor));
        prevBtn.setOnAction(e -> runner.prev(currentMonitor));
        nextBtn.setOnAction(e -> runner.next(currentMonitor));

        bar.getChildren().addAll(playBtn, pauseBtn, stopBtn, resetBtn, prevBtn, nextBtn);
        return bar;
    }

    private Button controlBtn(String symbol, String tooltip) {
        Button btn = new Button(symbol);
        btn.setStyle("-fx-background-color: #444; -fx-text-fill: white; -fx-font-size: 12; -fx-min-width: 28; -fx-min-height: 28;");
        btn.setTooltip(new Tooltip(tooltip));
        return btn;
    }

    public void show() {
        if (visible) return;
        hideDelay.stop();
        refresh();
        visible = true;
        TranslateTransition tt = new TranslateTransition(Duration.millis(200), panel);
        tt.setToX(0);
        tt.play();
    }

    public void hide() {
        if (!visible) return;
        hideDelay.stop();
        visible = false;
        TranslateTransition tt = new TranslateTransition(Duration.millis(200), panel);
        tt.setToX(PANEL_WIDTH);
        tt.play();
    }

    public void toggle() {
        if (visible) hide(); else show();
    }

    public boolean isVisible() { return visible; }

    public void setMonitor(String monitor) {
        if (monitor == null || monitor.isEmpty()) return;
        currentMonitor = monitor;
        headerLabel.setText("Playlist \u2014 " + monitor);
        refresh();
    }

    public void refresh() {
        if (currentMonitor.isEmpty()) return;
        entries.clear();
        imageCache.clear();
        selectedEntryIdx = -1;

        Playlist playlist = playlistManager.load(currentMonitor);
        if (playlist != null) {
            entries.addAll(playlist.getEntries());
        }

        Platform.runLater(() -> {
            entryList.getChildren().clear();
            for (int i = 0; i < entries.size(); i++) {
                entryList.getChildren().add(createEntryRow(i));
            }
            if (entries.isEmpty()) {
                Label empty = new Label("No entries. Use 'Add to Playlist' button.");
                empty.setTextFill(Color.GRAY);
                empty.setWrapText(true);
                empty.setFont(Font.font(11));
                entryList.getChildren().add(empty);
            }
        });
    }

    private Node createEntryRow(int idx) {
        WallpaperConfig cfg = entries.get(idx);

        ImageView thumb = new ImageView();
        thumb.setFitWidth(60);
        thumb.setFitHeight(34);
        thumb.setPreserveRatio(true);
        thumb.setCache(true);
        thumb.setCacheHint(CacheHint.SPEED);

        loadThumbAsync(idx, cfg, thumb);

        Label titleLabel = new Label(cfg.getWallpaperId());
        titleLabel.setTextFill(Color.LIGHTGRAY);
        titleLabel.setFont(Font.font(11));

        Button delBtn = new Button("\u2715");
        delBtn.setStyle("-fx-background-color: transparent; -fx-text-fill: #d9534f; -fx-font-size: 10;");
        delBtn.setTooltip(new Tooltip("Remove from playlist"));
        delBtn.setOnAction(e -> removeEntry(idx));

        HBox row = new HBox(8);
        row.setAlignment(Pos.CENTER_LEFT);
        row.setPadding(new Insets(4, 6, 4, 6));
        row.getStyleClass().add("playlist-entry");
        row.getChildren().addAll(thumb, titleLabel, new Region(), delBtn);
        HBox.setHgrow(titleLabel, Priority.ALWAYS);

        row.setUserData(idx);
        row.setOnMouseClicked(e -> selectEntry(idx));

        return row;
    }

    private void loadThumbAsync(int idx, WallpaperConfig cfg, ImageView thumb) {
        imageLoader.submit(() -> {
            WpprLib lib = WpprLib.getInstance();
            if (lib == null) return;
            Wppr wp = lib.findById(cfg.getWallpaperId());
            if (wp == null) return;

            String path = findPreviewPath(wp);
            if (path == null) return;

            Image img = new Image(Path.of(path).toUri().toString(), 80, 45, true, true, true);
            if (img.isError() || Thread.currentThread().isInterrupted()) return;

            imageCache.put(idx, img);
            Platform.runLater(() -> {
                thumb.setImage(img);
                FadeTransition ft = new FadeTransition(Duration.millis(200), thumb);
                ft.setFromValue(0);
                ft.setToValue(1);
                ft.play();
            });
        });
    }

    private String findPreviewPath(Wppr wp) {
        Path jpg = Paths.get(wp.getPreviewPath());
        if (Files.exists(jpg)) return jpg.toString();
        Path gif = Paths.get(wp.getPreviewPath().replace(".jpg", ".gif"));
        if (Files.exists(gif)) return gif.toString();
        return null;
    }

    private void selectEntry(int idx) {
        for (Node child : entryList.getChildren()) {
            child.getStyleClass().remove("selected");
        }
        if (idx >= 0 && idx < entryList.getChildren().size()) {
            Node row = entryList.getChildren().get(idx);
            if (!row.getStyleClass().contains("selected")) {
                row.getStyleClass().add("selected");
            }
        }
        selectedEntryIdx = idx;
    }

    private void removeEntry(int idx) {
        if (idx < 0 || idx >= entries.size()) return;
        Playlist playlist = playlistManager.load(currentMonitor);
        if (playlist != null) {
            playlist.getEntries().remove(idx);
            playlist.setCurrentIndex(Math.min(playlist.getCurrentIndex(), playlist.getEntries().size() - 1));
            playlistManager.save(currentMonitor, playlist);
            runner.reloadPlaylist(currentMonitor);
        }
        imageCache.clear();
        selectedEntryIdx = -1;
        refresh();
    }

    public void setupKeyboard(Node sceneRoot) {
        sceneRoot.addEventFilter(KeyEvent.KEY_PRESSED, event -> {
            if (!visible) return;
            KeyCode code = event.getCode();
            switch (code) {
                case UP:
                    if (entries.isEmpty()) break;
                    selectEntry(Math.max(0, selectedEntryIdx - 1));
                    event.consume();
                    break;
                case DOWN:
                    if (entries.isEmpty()) break;
                    selectEntry(Math.min(entries.size() - 1, selectedEntryIdx + 1));
                    event.consume();
                    break;
                case DELETE:
                    if (selectedEntryIdx >= 0 && selectedEntryIdx < entries.size()) {
                        removeEntry(selectedEntryIdx);
                    }
                    event.consume();
                    break;
                case SPACE:
                    if (runner.isPlaying(currentMonitor)) runner.pause(currentMonitor);
                    else runner.play(currentMonitor);
                    event.consume();
                    break;
                case N:
                    runner.next(currentMonitor);
                    event.consume();
                    break;
                case P:
                    runner.prev(currentMonitor);
                    event.consume();
                    break;
                default: break;
            }
        });
    }
}
