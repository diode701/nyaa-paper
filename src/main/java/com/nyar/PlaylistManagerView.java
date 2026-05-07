package com.nyar;

import com.nyar.config.Playlist;
import com.nyar.config.PlaylistManager;
import com.nyar.config.WallpaperConfig;
import com.nyar.wallpaper.PlaylistRunner;
import javafx.animation.FadeTransition;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.CacheHint;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
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
import java.util.function.Consumer;

public class PlaylistManagerView {

    private final Runnable onBack;
    private final PlaylistManager playlistManager = PlaylistManager.getInstance();
    private final PlaylistRunner runner = PlaylistRunner.getInstance();

    private VBox monitorListBox;
    private VBox entryListBox;
    private Label statusLabel;
    private String selectedMonitor;

    private ComboBox<String> modeCombo;
    private Spinner<Integer> intervalSpinner;
    private CheckBox loopCheck;
    private Label headerLabel;

    private int selectedEntryIdx = -1;
    private List<WallpaperConfig> currentEntries = new CopyOnWriteArrayList<>();

    private final ExecutorService imageLoader = Executors.newFixedThreadPool(2, r -> {
        Thread t = new Thread(r, "pmv-img");
        t.setDaemon(true);
        return t;
    });
    private final Map<Integer, Image> imageCache = new ConcurrentHashMap<>();

    public PlaylistManagerView(Runnable onBack) {
        this.onBack = onBack;
    }

    public Node getView() {
        BorderPane root = new BorderPane();
        root.setStyle("-fx-background-color: #1e1e1e;");

        root.setTop(createTopBar());

        HBox center = new HBox(0);
        center.setPadding(new Insets(10));

        monitorListBox = new VBox(5);
        monitorListBox.setPrefWidth(180);
        monitorListBox.setMinWidth(180);
        monitorListBox.setPadding(new Insets(5));
        monitorListBox.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5;");
        ScrollPane monitorScroll = new ScrollPane(monitorListBox);
        monitorScroll.setFitToWidth(true);
        monitorScroll.setHbarPolicy(ScrollPane.ScrollBarPolicy.NEVER);
        monitorScroll.setStyle("-fx-background: #2a2a2a; -fx-background-color: transparent;");

        entryListBox = new VBox(5);
        entryListBox.setPadding(new Insets(5, 5, 5, 10));
        ScrollPane entryScroll = new ScrollPane(entryListBox);
        entryScroll.setFitToWidth(true);
        entryScroll.setStyle("-fx-background: #1e1e1e; -fx-background-color: transparent;");
        HBox.setHgrow(entryScroll, Priority.ALWAYS);

        center.getChildren().addAll(monitorScroll, entryScroll);
        root.setCenter(center);

        root.setBottom(createControls());

        refreshMonitorList();
        setupKeyboardShortcuts(root);

        return root;
    }

    private HBox createTopBar() {
        HBox topBar = new HBox(10);
        topBar.setPadding(new Insets(15));
        topBar.setAlignment(Pos.CENTER_LEFT);
        topBar.setStyle("-fx-background-color: #2d2d2d;");

        Button backButton = new Button("← Back");
        backButton.setStyle("-fx-background-color: #444; -fx-text-fill: white;");
        backButton.setOnAction(e -> onBack.run());

        headerLabel = new Label("Playlist Manager");
        headerLabel.setTextFill(Color.WHITE);
        headerLabel.setStyle("-fx-font-size: 16; -fx-font-weight: bold;");

        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);

        Label shortcutHint = new Label("Esc/B: back | ↑↓: navigate | Del: remove | Space: play/pause");
        shortcutHint.setTextFill(Color.GRAY);
        shortcutHint.setFont(Font.font(10));

        topBar.getChildren().addAll(backButton, headerLabel, spacer, shortcutHint);
        return topBar;
    }

    private HBox createControls() {
        HBox bar = new HBox(10);
        bar.setPadding(new Insets(10, 15, 10, 15));
        bar.setAlignment(Pos.CENTER_LEFT);
        bar.setStyle("-fx-background-color: #2d2d2d;");

        Button playBtn = controlBtn("▶ Play", "#4a90e2");
        Button pauseBtn = controlBtn("⏸ Pause", "#f0ad4e");
        Button stopBtn = controlBtn("⏹ Stop", "#d9534f");
        Button resetBtn = controlBtn("↺ Reset", "#999");
        Button prevBtn = controlBtn("⏮ Prev", "#888");
        Button nextBtn = controlBtn("Next ⏭", "#888");

        playBtn.setOnAction(e -> runOnMonitor(runner::play));
        pauseBtn.setOnAction(e -> runOnMonitor(m -> {
            if (runner.isPaused(m)) runner.resume(m); else runner.pause(m);
        }));
        stopBtn.setOnAction(e -> runOnMonitor(runner::stop));
        resetBtn.setOnAction(e -> runOnMonitor(runner::reset));
        prevBtn.setOnAction(e -> runOnMonitor(runner::prev));
        nextBtn.setOnAction(e -> runOnMonitor(runner::next));

        bar.getChildren().addAll(playBtn, pauseBtn, stopBtn, new Separator(), resetBtn, prevBtn, nextBtn);

        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);

        Label modeLabel = new Label("Mode:");
        modeLabel.setTextFill(Color.LIGHTGRAY);
        modeCombo = new ComboBox<>();
        modeCombo.getItems().addAll("SEQUENTIAL", "RANDOM");
        modeCombo.setValue("SEQUENTIAL");
        modeCombo.setOnAction(e -> saveModeAndInterval());

        Label intervalLabel = new Label("Interval (s):");
        intervalLabel.setTextFill(Color.LIGHTGRAY);
        intervalSpinner = new Spinner<>(5, 3600, 60);
        intervalSpinner.setEditable(true);
        intervalSpinner.setPrefWidth(80);
        intervalSpinner.valueProperty().addListener((obs, oldVal, newVal) -> saveModeAndInterval());

        loopCheck = new CheckBox("Loop");
        loopCheck.setTextFill(Color.LIGHTGRAY);
        loopCheck.setSelected(true);
        loopCheck.setOnAction(e -> saveModeAndInterval());

        statusLabel = new Label("");
        statusLabel.setTextFill(Color.LIGHTGRAY);

        bar.getChildren().addAll(spacer, modeLabel, modeCombo, intervalLabel, intervalSpinner, loopCheck, statusLabel);
        return bar;
    }

    private Button controlBtn(String text, String color) {
        Button btn = new Button(text);
        btn.setStyle("-fx-background-color: " + color + "; -fx-text-fill: white; -fx-font-size: 11;");
        btn.setFocusTraversable(true);
        return btn;
    }

    private void runOnMonitor(Consumer<String> action) {
        if (selectedMonitor != null && !selectedMonitor.isEmpty()) {
            action.accept(selectedMonitor);
        } else {
            statusLabel.setText("Select a monitor first");
        }
    }

    private void refreshMonitorList() {
        monitorListBox.getChildren().clear();
        List<String> monitors = playlistManager.listMonitors();

        if (monitors.isEmpty()) {
            Label empty = new Label("No playlists yet.\nAdd entries from\nProperties view.");
            empty.setTextFill(Color.GRAY);
            empty.setFont(Font.font(11));
            empty.setWrapText(true);
            empty.setPadding(new Insets(10));
            monitorListBox.getChildren().add(empty);
            return;
        }

        for (String mon : monitors) {
            Button monBtn = new Button(mon);
            monBtn.setMaxWidth(Double.MAX_VALUE);
            monBtn.setAlignment(Pos.CENTER_LEFT);
            monBtn.setStyle("-fx-background-color: #3a3a3a; -fx-text-fill: white;");
            monBtn.setOnAction(e -> selectMonitor(mon));
            monitorListBox.getChildren().add(monBtn);
        }
    }

    private void selectMonitor(String monitor) {
        selectedMonitor = monitor;
        headerLabel.setText("Playlist Manager — " + monitor);

        for (Node child : monitorListBox.getChildren()) {
            child.setStyle("-fx-background-color: #3a3a3a; -fx-text-fill: white;");
        }

        entryListBox.getChildren().clear();
        imageCache.clear();
        currentEntries.clear();
        selectedEntryIdx = -1;

        Playlist playlist = playlistManager.load(monitor);
        if (playlist == null) {
            Label empty = new Label("No playlist loaded");
            empty.setTextFill(Color.GRAY);
            entryListBox.getChildren().add(empty);
            return;
        }

        currentEntries.addAll(playlist.getEntries());
        modeCombo.setValue(playlist.getMode().name());
        intervalSpinner.getValueFactory().setValue(playlist.getIntervalSec());
        loopCheck.setSelected(playlist.isLoop());
        loopCheck.setSelected(playlist.isLoop());

        for (int i = 0; i < currentEntries.size(); i++) {
            entryListBox.getChildren().add(createEntryRow(i, monitor));
        }

        if (currentEntries.isEmpty()) {
            Label empty = new Label("No entries in playlist");
            empty.setTextFill(Color.GRAY);
            entryListBox.getChildren().add(empty);
        }
    }

    private Node createEntryRow(int idx, String monitor) {
        WallpaperConfig cfg = currentEntries.get(idx);

        ImageView thumb = new ImageView();
        thumb.setFitWidth(80);
        thumb.setFitHeight(45);
        thumb.setPreserveRatio(true);
        thumb.setCache(true);
        thumb.setCacheHint(CacheHint.SPEED);

        loadThumbAsync(idx, cfg, thumb);

        WpprLib lib = WpprLib.getInstance();
        String title = cfg.getWallpaperId();
        if (lib != null) {
            Wppr wp = lib.findById(cfg.getWallpaperId());
            if (wp != null) title = wp.getTitle();
        }

        Label titleLabel = new Label(title);
        titleLabel.setTextFill(Color.LIGHTGRAY);
        titleLabel.setFont(Font.font(12));
        titleLabel.setMaxWidth(300);
        titleLabel.setWrapText(true);

        Button upBtn = new Button("↑");
        upBtn.setStyle("-fx-background-color: #444; -fx-text-fill: white; -fx-font-size: 10; -fx-min-width: 22;");
        upBtn.setOnAction(e -> moveEntry(idx, -1, monitor));

        Button downBtn = new Button("↓");
        downBtn.setStyle("-fx-background-color: #444; -fx-text-fill: white; -fx-font-size: 10; -fx-min-width: 22;");
        downBtn.setOnAction(e -> moveEntry(idx, 1, monitor));

        Button delBtn = new Button("✕");
        delBtn.setStyle("-fx-background-color: transparent; -fx-text-fill: #d9534f; -fx-font-size: 12;");
        delBtn.setTooltip(new Tooltip("Remove from playlist"));
        delBtn.setOnAction(e -> removeEntry(idx, monitor));

        VBox orderBtns = new VBox(0);
        orderBtns.getChildren().addAll(upBtn, downBtn);

        HBox row = new HBox(10);
        row.setAlignment(Pos.CENTER_LEFT);
        row.setPadding(new Insets(6));
        row.getStyleClass().add("playlist-entry");
        row.getChildren().addAll(thumb, titleLabel, new Region(), orderBtns, delBtn);
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
        for (Node child : entryListBox.getChildren()) {
            child.getStyleClass().remove("selected");
        }
        if (idx >= 0 && idx < entryListBox.getChildren().size()) {
            Node row = entryListBox.getChildren().get(idx);
            row.getStyleClass().add("selected");
        }
        selectedEntryIdx = idx;
    }

    private void moveEntry(int idx, int delta, String monitor) {
        if (idx < 0 || idx >= currentEntries.size()) return;
        int newIdx = idx + delta;
        if (newIdx < 0 || newIdx >= currentEntries.size()) return;

        WallpaperConfig moved = currentEntries.remove(idx);
        currentEntries.add(newIdx, moved);

        Playlist playlist = playlistManager.load(monitor);
        if (playlist != null) {
            playlist.getEntries().clear();
            playlist.getEntries().addAll(currentEntries);
            playlistManager.save(monitor, playlist);
            runner.reloadPlaylist(monitor);
        }

        entryListBox.getChildren().clear();
        imageCache.clear();
        selectedEntryIdx = -1;
        for (int i = 0; i < currentEntries.size(); i++) {
            entryListBox.getChildren().add(createEntryRow(i, monitor));
        }
    }

    private void removeEntry(int idx, String monitor) {
        if (idx < 0 || idx >= currentEntries.size()) return;
        currentEntries.remove(idx);

        Playlist playlist = playlistManager.load(monitor);
        if (playlist != null) {
            playlist.getEntries().clear();
            playlist.getEntries().addAll(currentEntries);
            playlist.setCurrentIndex(Math.min(playlist.getCurrentIndex(), currentEntries.size() - 1));
            playlistManager.save(monitor, playlist);
            runner.reloadPlaylist(monitor);
        }

        imageCache.clear();
        selectedEntryIdx = -1;
        entryListBox.getChildren().clear();
        for (int i = 0; i < currentEntries.size(); i++) {
            entryListBox.getChildren().add(createEntryRow(i, monitor));
        }
    }

    private void saveModeAndInterval() {
        if (selectedMonitor == null || selectedMonitor.isEmpty()) return;
        Playlist playlist = playlistManager.load(selectedMonitor);
        if (playlist == null) return;
        try {
            playlist.setMode(Playlist.Mode.valueOf(modeCombo.getValue()));
        } catch (IllegalArgumentException ignored) {}
        playlist.setIntervalSec(intervalSpinner.getValue());
        playlist.setLoop(loopCheck.isSelected());
        playlistManager.save(selectedMonitor, playlist);
        runner.reloadPlaylist(selectedMonitor);
    }

    private void setupKeyboardShortcuts(Node root) {
        root.setOnKeyPressed(event -> {
            KeyCode code = event.getCode();
            switch (code) {
                case ESCAPE:
                case B:
                    if (!event.isControlDown()) {
                        onBack.run();
                        event.consume();
                    }
                    break;
                case UP:
                    if (!currentEntries.isEmpty()) {
                        selectEntry(Math.max(0, selectedEntryIdx - 1));
                        event.consume();
                    }
                    break;
                case DOWN:
                    if (!currentEntries.isEmpty()) {
                        selectEntry(Math.min(currentEntries.size() - 1, selectedEntryIdx + 1));
                        event.consume();
                    }
                    break;
                case DELETE:
                    if (selectedEntryIdx >= 0 && selectedMonitor != null) {
                        removeEntry(selectedEntryIdx, selectedMonitor);
                        event.consume();
                    }
                    break;
                case SPACE:
                    if (selectedMonitor != null) {
                        if (runner.isPlaying(selectedMonitor)) runner.pause(selectedMonitor);
                        else runner.play(selectedMonitor);
                        event.consume();
                    }
                    break;
                default: break;
            }
        });
    }
}
