package com.nyar;

import com.nyar.config.*;
import com.nyar.settings.*;
import com.nyar.wallpaper.GpuConfig;
import com.nyar.wallpaper.PlaylistRunner;
import com.nyar.wallpaper.WallpaperManager;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyCodeCombination;
import javafx.scene.input.KeyCombination;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class WallpaperPropertiesView {
    private final Wppr wallpaper;
    private final Runnable onBack;
    private final WallpaperManager wallpaperManager = WallpaperManager.getInstance();
    private final PlaylistManager playlistManager = PlaylistManager.getInstance();
    private final PlaylistRunner playlistRunner = PlaylistRunner.getInstance();

    private VBox propertiesContainer;
    private VBox globalContainer;
    private Label statusLabel;

    private ComboBox<String> monitorCombo;
    private RadioButton defaultRb, driRb, nvidiaRb;
    private Spinner<Integer> driSpinner;
    private CheckBox glThreadedCheck;

    private final List<Setting> allSettings = new ArrayList<>();
    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    private CheckBox screenshotCheck;
    private TextField screenshotPathField;
    private Spinner<Integer> screenshotDelaySpinner;

    private MiniPlaylistPanel miniPanel;

    public void shutdown() {
        executor.shutdownNow();
        try {
            if (!executor.awaitTermination(1, TimeUnit.SECONDS)) {}
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
    }

    public WallpaperPropertiesView(Wppr wallpaper, Runnable onBack) {
        this.wallpaper = wallpaper;
        this.onBack = onBack;
    }

    public Node getView() {
        BorderPane root = new BorderPane();
        root.setStyle("-fx-background-color: #1e1e1e;");

        root.setTop(createTopBar());

        TabPane tabPane = new TabPane();
        tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE);
        tabPane.setStyle("-fx-background-color: #2d2d2d;");

        Tab customTab = new Tab("Custom Properties");
        propertiesContainer = new VBox(15);
        propertiesContainer.setPadding(new Insets(20));
        ScrollPane customScroll = new ScrollPane(propertiesContainer);
        customScroll.setFitToWidth(true);
        customScroll.setStyle("-fx-background: #1e1e1e; -fx-background-color: transparent;");
        customTab.setContent(customScroll);

        Tab globalTab = new Tab("Global Options");
        globalContainer = new VBox(15);
        globalContainer.setPadding(new Insets(20));
        ScrollPane globalScroll = new ScrollPane(globalContainer);
        globalScroll.setFitToWidth(true);
        globalScroll.setStyle("-fx-background: #1e1e1e; -fx-background-color: transparent;");
        globalTab.setContent(globalScroll);

        tabPane.getTabs().addAll(customTab, globalTab);
        root.setCenter(tabPane);

        VBox bottomContainer = new VBox(5);
        bottomContainer.getChildren().addAll(
                createGpuSection(),
                createScreenshotSection(),
                createBottomBar()
        );
        root.setBottom(bottomContainer);

        loadCustomPropertiesFromJSON();
        populateGlobalSettings();

        wallpaperManager.setCrashListener((monitor, exitCode) -> {
            Platform.runLater(() ->
                statusLabel.setText("Crashed on " + monitor + " (exit " + exitCode + "). Try reapplying."));
        });

        setupKeyboardShortcuts(root);
        setupMiniPanel(root);

        return root;
    }

    private HBox createTopBar() {
        HBox topBar = new HBox(10);
        topBar.setPadding(new Insets(15));
        topBar.setAlignment(Pos.CENTER_LEFT);
        topBar.setStyle("-fx-background-color: #2d2d2d;");

        Button backButton = new Button("← Back");
        backButton.setOnAction(e -> {
            executor.shutdownNow();
            onBack.run();
        });
        backButton.setFocusTraversable(true);
        backButton.setTooltip(new Tooltip("Go back (Esc)"));

        Settings settings = SettingsManager.load();
        List<String> monitors = settings.getMonitors();
        monitorCombo = new ComboBox<>();
        monitorCombo.setEditable(true);
        monitorCombo.getItems().addAll(monitors);
        if (!monitors.isEmpty()) {
            monitorCombo.setValue(monitors.get(0));
        }
        monitorCombo.setTooltip(new Tooltip("Select or enter monitor name (1~9 to pick)"));
        monitorCombo.setFocusTraversable(true);
        monitorCombo.setPrefWidth(140);
        monitorCombo.valueProperty().addListener((obs, oldVal, newVal) -> {
            if (miniPanel != null && newVal != null && !newVal.isEmpty()) {
                miniPanel.setMonitor(newVal);
            }
        });

        Button loadBtn = new Button("Load last");
        loadBtn.setStyle("-fx-background-color: #444; -fx-text-fill: white;");
        loadBtn.setTooltip(new Tooltip("Load last applied settings for this wallpaper on the selected monitor (L)"));
        loadBtn.setOnAction(e -> loadSavedSettings());

        Button addPlaylistBtn = new Button("Add to Playlist");
        addPlaylistBtn.setStyle("-fx-background-color: #3a7d44; -fx-text-fill: white;");
        addPlaylistBtn.setTooltip(new Tooltip("Add current settings as playlist entry for the selected monitor (A)"));
        addPlaylistBtn.setOnAction(e -> addToPlaylist());

        Button togglePanelBtn = new Button("▶");
        togglePanelBtn.setStyle("-fx-background-color: #444; -fx-text-fill: white; -fx-min-width: 30;");
        togglePanelBtn.setTooltip(new Tooltip("Toggle playlist panel (Ctrl+L)"));
        togglePanelBtn.setOnAction(e -> {
            if (miniPanel != null) miniPanel.toggle();
        });

        Label titleLabel = new Label("Properties: " + wallpaper.getTitle());
        titleLabel.setTextFill(Color.WHITE);
        titleLabel.setStyle("-fx-font-size: 16; -fx-font-weight: bold;");

        Region spacer = new Region();
        HBox.setHgrow(spacer, Priority.ALWAYS);

        topBar.getChildren().addAll(backButton, monitorCombo, loadBtn, addPlaylistBtn, togglePanelBtn, spacer, titleLabel);
        return topBar;
    }

    private VBox createGpuSection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5; -fx-padding: 10;");

        Label header = new Label("GPU");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(16));

        boolean nvidiaAvailable = SettingsManager.load().isNvidiaAvailable();
        ToggleGroup group = new ToggleGroup();

        defaultRb = new RadioButton("Default (auto)");
        defaultRb.setToggleGroup(group);
        defaultRb.setSelected(true);
        defaultRb.setTextFill(Color.LIGHTGRAY);

        driRb = new RadioButton("DRI_PRIME (AMD/Intel)");
        driRb.setToggleGroup(group);
        driRb.setTextFill(Color.LIGHTGRAY);

        HBox driRow = new HBox(10);
        driRow.setAlignment(Pos.CENTER_LEFT);
        driSpinner = new Spinner<>(0, 9, 1);
        driSpinner.setEditable(true);
        driSpinner.disableProperty().bind(driRb.selectedProperty().not());
        Label driLabel = new Label("GPU index:");
        driLabel.setTextFill(Color.LIGHTGRAY);
        driRow.getChildren().addAll(driLabel, driSpinner);

        nvidiaRb = new RadioButton("NVIDIA Prime Offload");
        nvidiaRb.setToggleGroup(group);
        nvidiaRb.setTextFill(Color.LIGHTGRAY);
        nvidiaRb.setVisible(nvidiaAvailable);
        nvidiaRb.setManaged(nvidiaAvailable);

        glThreadedCheck = new CheckBox("Disable GL threaded optimizations (fixes GLFW/OpenGL issues)");
        glThreadedCheck.setTextFill(Color.LIGHTGRAY);

        box.getChildren().addAll(header, defaultRb, driRb, driRow, nvidiaRb, glThreadedCheck);
        return box;
    }

    private VBox createBottomBar() {
        VBox bottomBox = new VBox(10);
        bottomBox.setPadding(new Insets(15));
        bottomBox.setStyle("-fx-background-color: #2d2d2d;");

        statusLabel = new Label("Ready");
        statusLabel.setTextFill(Color.LIGHTGRAY);
        statusLabel.setFocusTraversable(false);

        HBox buttonRow = new HBox(10);
        buttonRow.setAlignment(Pos.CENTER_RIGHT);

        Button applyButton = new Button("Apply to Monitor");
        applyButton.setStyle("-fx-background-color: #4a90e2; -fx-text-fill: white; -fx-font-weight: bold;");
        applyButton.setTooltip(new Tooltip("Apply current settings to selected monitor (Ctrl+Enter)"));
        applyButton.setFocusTraversable(true);
        applyButton.setOnAction(e -> applyToMonitor());

        Button stopButton = new Button("Stop on Monitor");
        stopButton.setStyle("-fx-background-color: #d9534f; -fx-text-fill: white;");
        stopButton.setTooltip(new Tooltip("Stop wallpaper on selected monitor (S)"));
        stopButton.setFocusTraversable(true);
        stopButton.setOnAction(e -> stopOnMonitor());

        buttonRow.getChildren().addAll(applyButton, stopButton);

        bottomBox.getChildren().addAll(statusLabel, buttonRow);
        return bottomBox;
    }

    private void setupMiniPanel(Node root) {
        miniPanel = new MiniPlaylistPanel();
        String monitor = monitorCombo.getValue();
        if (monitor != null && !monitor.isEmpty()) {
            miniPanel.setMonitor(monitor);
        }
        if (root instanceof BorderPane) {
            BorderPane bp = (BorderPane) root;
            Node center = bp.getCenter();
            StackPane overlay = new StackPane(center, miniPanel.getView());
            overlay.setPickOnBounds(false);
            bp.setCenter(overlay);
        }
        miniPanel.setupKeyboard(root);
    }

    private void setupKeyboardShortcuts(Node root) {
        final KeyCombination ctrlEnter = new KeyCodeCombination(KeyCode.ENTER, KeyCombination.CONTROL_DOWN);
        final KeyCombination ctrlUp = new KeyCodeCombination(KeyCode.UP, KeyCombination.CONTROL_DOWN);
        final KeyCombination ctrlDown = new KeyCodeCombination(KeyCode.DOWN, KeyCombination.CONTROL_DOWN);
        final KeyCombination ctrlL = new KeyCodeCombination(KeyCode.L, KeyCombination.CONTROL_DOWN);

        root.setOnKeyPressed(event -> {
            if (ctrlEnter.match(event)) {
                applyToMonitor();
                event.consume();
            } else if (ctrlL.match(event)) {
                if (miniPanel != null) miniPanel.toggle();
                event.consume();
            } else if (ctrlUp.match(event)) {
                cycleMonitor(-1);
                event.consume();
            } else if (ctrlDown.match(event)) {
                cycleMonitor(1);
                event.consume();
            } else if (event.getCode() == KeyCode.ESCAPE) {
                onBack.run();
                event.consume();
            } else if (event.getCode() == KeyCode.A && !event.isControlDown()) {
                addToPlaylist();
                event.consume();
            } else if (event.getCode() == KeyCode.S && !event.isControlDown()) {
                stopOnMonitor();
                event.consume();
            } else if (event.getCode() == KeyCode.L && !event.isControlDown()) {
                loadSavedSettings();
                event.consume();
            } else if (event.getCode().isDigitKey() && !event.isControlDown()) {
                int digit = Integer.parseInt(event.getCode().getName());
                selectMonitorByIndex(digit);
                event.consume();
            } else if (event.getCode() == KeyCode.DIGIT0 && !event.isControlDown()) {
                selectMonitorByIndex(10);
                event.consume();
            }
        });
    }

    private void cycleMonitor(int delta) {
        int count = monitorCombo.getItems().size();
        if (count == 0) return;
        String current = monitorCombo.getValue();
        int idx = monitorCombo.getItems().indexOf(current);
        if (idx < 0) idx = 0;
        idx = (idx + delta + count) % count;
        monitorCombo.setValue(monitorCombo.getItems().get(idx));
    }

    private void selectMonitorByIndex(int num) {
        List<String> items = monitorCombo.getItems();
        int idx = num - 1;
        if (idx >= 0 && idx < items.size()) {
            monitorCombo.setValue(items.get(idx));
        }
    }

    private void loadCustomPropertiesFromJSON() {
        if (!wallpaper.hasProperties()) {
            Label noProps = new Label("This wallpaper has no customizable properties.");
            noProps.setTextFill(Color.LIGHTGRAY);
            propertiesContainer.getChildren().add(noProps);
            return;
        }

        statusLabel.setText("Loading custom properties...");
        executor.submit(() -> {
            try {
                List<Setting> props = PropertyParser.parseFromWppr(wallpaper);
                Platform.runLater(() -> {
                    propertiesContainer.getChildren().clear();
                    if (props.isEmpty()) {
                        Label empty = new Label("No properties found in JSON.");
                        empty.setTextFill(Color.LIGHTGRAY);
                        propertiesContainer.getChildren().add(empty);
                        return;
                    }
                    for (Setting prop : props) {
                        allSettings.add(prop);
                        VBox propBox = createSettingBox(prop.createControl(s -> {
                            statusLabel.setText("Property changed (not yet applied)");
                        }));
                        propertiesContainer.getChildren().add(propBox);
                    }
                    statusLabel.setText("Loaded " + props.size() + " custom properties");
                });
            } catch (Exception e) {
                Platform.runLater(() -> {
                    statusLabel.setText("Error parsing properties: " + e.getMessage());
                    Label error = new Label("Failed to load custom properties.\n" + e.getMessage());
                    error.setTextFill(Color.RED);
                    propertiesContainer.getChildren().setAll(error);
                });
            }
        });
    }

    private void populateGlobalSettings() {
        List<GlobalOption> globals = GlobalOptionsFactory.createAll();

        for (GlobalOption opt : globals) {
            allSettings.add(opt);
            Node control = opt.createControl(s -> statusLabel.setText("Global option changed (not yet applied)"));
            VBox optBox = createSettingBox(control);
            globalContainer.getChildren().add(optBox);
        }
    }

    private VBox createSettingBox(Node control) {
        VBox box = new VBox(5);
        box.setPadding(new Insets(10));
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5;");
        box.setFocusTraversable(false);
        box.getChildren().add(control);
        return box;
    }

    private GpuConfig buildGpuConfig() {
        GpuConfig gpuConfig = new GpuConfig();
        if (defaultRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.DEFAULT);
        } else if (driRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.DRI_PRIME);
            gpuConfig.setDriPrimeNumber(driSpinner.getValue());
        } else if (nvidiaRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.NVIDIA);
        }
        gpuConfig.setGlThreadedOpt(glThreadedCheck.isSelected());
        return gpuConfig;
    }

    private WallpaperConfig captureConfig(String monitor) {
        GpuConfig gpuConfig = buildGpuConfig();
        WallpaperConfig config = new WallpaperConfig(wallpaper.getId(), monitor, getProjectJsonTimestamp());
        config.setGpuMode(gpuConfig.getMode().name());
        config.setDriPrimeNumber(gpuConfig.getDriPrimeNumber());
        config.setGlThreadedOpt(gpuConfig.isGlThreadedOpt());
        for (Setting s : allSettings) {
            String val = s.getValueString();
            if (val != null) {
                config.put(s.getName(), val);
            }
        }
        return config;
    }

    private void applyToMonitor() {
        String monitor = monitorCombo.getValue();
        if (monitor == null || monitor.trim().isEmpty()) {
            statusLabel.setText("Please select a monitor.");
            return;
        }
        String mon = monitor.trim();

        if (playlistRunner.isPlaying(mon)) {
            playlistRunner.pause(mon);
            statusLabel.setText("Playlist paused — manual apply on " + mon);
        }

        GpuConfig gpuConfig = buildGpuConfig();

        String screenshotPath;
        int screenshotDelay;
        if (screenshotCheck.isSelected()) {
            screenshotPath = screenshotPathField.getText().trim();
            if (screenshotPath.isEmpty()) {
                statusLabel.setText("Please enter a screenshot output path.");
                return;
            }
            screenshotDelay = screenshotDelaySpinner.getValue();
        } else {
            screenshotDelay = 5;
            screenshotPath = null;
        }

        statusLabel.setText("Applying...");

        WallpaperConfig config = captureConfig(mon);

        executor.submit(() -> {
            try {
                wallpaperManager.apply(mon, wallpaper.getId(), allSettings,
                        gpuConfig, screenshotPath, screenshotDelay);
                WallpaperConfigManager.getInstance().save(config);
                Platform.runLater(() -> statusLabel.setText("Applied to " + mon));
            } catch (Exception e) {
                Platform.runLater(() -> statusLabel.setText("Error: " + e.getMessage()));
            }
        });
    }

    private void addToPlaylist() {
        String monitor = monitorCombo.getValue();
        if (monitor == null || monitor.trim().isEmpty()) {
            statusLabel.setText("Please select a monitor first.");
            return;
        }
        String mon = monitor.trim();
        WallpaperConfig entry = captureConfig(mon);

        executor.submit(() -> {
            Playlist playlist = playlistManager.load(mon);
            if (playlist == null) {
                playlist = new Playlist();
            }
            playlist.getEntries().add(entry);
            playlistManager.save(mon, playlist);
            int entryCount = playlist.getEntries().size();

            Platform.runLater(() -> {
                statusLabel.setText("Added to " + mon + " playlist (" + entryCount + " entries)");
                if (miniPanel != null && miniPanel.isVisible()) {
                    miniPanel.setMonitor(mon);
                }
            });
        });
    }

    private long getProjectJsonTimestamp() {
        try {
            return Files.getLastModifiedTime(
                    Paths.get(wallpaper.getBasePath(), "project.json")).toMillis();
        } catch (IOException e) {
            return 0;
        }
    }

    private VBox createScreenshotSection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5; -fx-padding: 10;");

        Label header = new Label("📸 Screenshot (for PyWAL etc.)");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(16));

        screenshotCheck = new CheckBox("Generate screenshot on apply");
        screenshotCheck.setTextFill(Color.LIGHTGRAY);

        HBox pathRow = new HBox(10);
        pathRow.setAlignment(Pos.CENTER_LEFT);
        Label pathLabel = new Label("Output file:");
        pathLabel.setTextFill(Color.LIGHTGRAY);

        String defaultScreenshotPath = SettingsManager.load().getScreenshotPath();
        screenshotPathField = new TextField(defaultScreenshotPath);
        screenshotPathField.setPromptText("/path/to/screenshot.png");
        screenshotPathField.setPrefWidth(300);
        screenshotPathField.disableProperty().bind(screenshotCheck.selectedProperty().not());
        pathRow.getChildren().addAll(pathLabel, screenshotPathField);

        HBox delayRow = new HBox(10);
        delayRow.setAlignment(Pos.CENTER_LEFT);
        Label delayLabel = new Label("Delay (frames):");
        delayLabel.setTextFill(Color.LIGHTGRAY);
        screenshotDelaySpinner = new Spinner<>(0, 100, 5);
        screenshotDelaySpinner.setEditable(true);
        screenshotDelaySpinner.disableProperty().bind(screenshotCheck.selectedProperty().not());
        delayRow.getChildren().addAll(delayLabel, screenshotDelaySpinner);

        box.getChildren().addAll(header, screenshotCheck, pathRow, delayRow);
        return box;
    }

    private void loadSavedSettings() {
        String monitor = monitorCombo.getValue();
        if (monitor == null || monitor.trim().isEmpty()) {
            statusLabel.setText("Please select a monitor first.");
            return;
        }

        WallpaperConfig saved = WallpaperConfigManager.getInstance()
                .load(monitor.trim(), wallpaper.getId());

        if (saved == null || saved.getSettings().isEmpty()) {
            statusLabel.setText("No saved settings for this wallpaper on " + monitor.trim());
            return;
        }

        Map<String, String> values = saved.getSettings();
        int applied = 0;
        for (Setting s : allSettings) {
            String key = s.getName();
            if (values.containsKey(key)) {
                s.setValueFromString(values.get(key));
                applied++;
            }
        }

        if (applied > 0) {
            rebuildUI();
            statusLabel.setText("Loaded " + applied + " saved settings for " + monitor.trim());
        } else {
            statusLabel.setText("Saved config found but no matching settings.");
        }
    }

    private void rebuildUI() {
        propertiesContainer.getChildren().clear();
        globalContainer.getChildren().clear();

        for (Setting s : allSettings) {
            Node control = s.createControl(ignored ->
                    statusLabel.setText("Property changed (not yet applied)"));
            VBox box = createSettingBox(control);
            if (s instanceof GlobalOption) {
                globalContainer.getChildren().add(box);
            } else {
                propertiesContainer.getChildren().add(box);
            }
        }
    }

    private void stopOnMonitor() {
        String monitor = monitorCombo.getValue();
        if (monitor == null || monitor.trim().isEmpty()) {
            statusLabel.setText("Please select a monitor.");
            return;
        }
        wallpaperManager.stop(monitor.trim());
        statusLabel.setText("Stopped on " + monitor);
    }
}
