package com.nyar;

import com.nyar.config.Settings;
import com.nyar.config.SettingsManager;
import com.nyar.settings.*;
import com.nyar.wallpaper.GpuConfig;
import com.nyar.wallpaper.WallpaperManager;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyCodeCombination;
import javafx.scene.input.KeyCombination;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.HBox;
import javafx.scene.layout.Priority;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

public class WallpaperPropertiesView {
    private final Wppr wallpaper;
    private final Runnable onBack;
    private final WallpaperManager wallpaperManager = WallpaperManager.getInstance();

    private VBox propertiesContainer;
    private VBox globalContainer;
    private Label statusLabel;

    // Monitor & GPU selection
    private ComboBox<String> monitorCombo;
    private RadioButton defaultRb, driRb, nvidiaRb;
    private Spinner<Integer> driSpinner;

    private final List<Setting> allSettings = new ArrayList<>();
    private final ExecutorService executor = Executors.newSingleThreadExecutor();



    // Screenshot controls
    private CheckBox screenshotCheck;
    private TextField screenshotPathField;
    private Spinner<Integer> screenshotDelaySpinner;

    public void shutdown() {
        executor.shutdownNow();  // interrupt running tasks
        try {
            if (!executor.awaitTermination(1, TimeUnit.SECONDS)) {
                // optionally log
            }
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

    // Top bar
    root.setTop(createTopBar());

    // Tab pane
    TabPane tabPane = new TabPane();
    tabPane.setTabClosingPolicy(TabPane.TabClosingPolicy.UNAVAILABLE);
    tabPane.setStyle("-fx-background-color: #2d2d2d;");

    // Custom properties tab
    Tab customTab = new Tab("Custom Properties");
    propertiesContainer = new VBox(15);
    propertiesContainer.setPadding(new Insets(20));
    ScrollPane customScroll = new ScrollPane(propertiesContainer);
    customScroll.setFitToWidth(true);
    customScroll.setStyle("-fx-background: #1e1e1e; -fx-background-color: transparent;");
    customTab.setContent(customScroll);

    // Global options tab
    Tab globalTab = new Tab("Global Options");
    globalContainer = new VBox(15);
    globalContainer.setPadding(new Insets(20));
    ScrollPane globalScroll = new ScrollPane(globalContainer);
    globalScroll.setFitToWidth(true);
    globalScroll.setStyle("-fx-background: #1e1e1e; -fx-background-color: transparent;");
    globalTab.setContent(globalScroll);

    tabPane.getTabs().addAll(customTab, globalTab);
    root.setCenter(tabPane);

    // Bottom area: GPU section + Screenshot section + buttons
    VBox bottomContainer = new VBox(5);
    bottomContainer.getChildren().addAll(
            createMonitorGpuSection(),
            createScreenshotSection(),
            createBottomBar()
    );
    root.setBottom(bottomContainer);

    // Load data
    loadCustomPropertiesFromJSON();
    populateGlobalSettings();

    // Set up keyboard shortcuts
    setupKeyboardShortcuts(root);

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

        Label titleLabel = new Label("Properties: " + wallpaper.getTitle());
        titleLabel.setTextFill(Color.WHITE);
        titleLabel.setStyle("-fx-font-size: 16; -fx-font-weight: bold;");

        topBar.getChildren().addAll(backButton, titleLabel);
        return topBar;
    }

    private VBox createMonitorGpuSection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5; -fx-padding: 10;");

        Label header = new Label("Monitor & GPU");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(16));

        // Monitor combo
        Settings settings = SettingsManager.load();
        List<String> monitors = settings.getMonitors();
        monitorCombo = new ComboBox<>();
        monitorCombo.setEditable(true);
        monitorCombo.getItems().addAll(monitors);
        if (!monitors.isEmpty()) {
            monitorCombo.setValue(monitors.get(0));
        }
        monitorCombo.setTooltip(new Tooltip("Select or enter monitor name"));
        monitorCombo.setFocusTraversable(true);
        monitorCombo.setMaxWidth(Double.MAX_VALUE);

        HBox monitorRow = new HBox(10);
        monitorRow.setAlignment(Pos.CENTER_LEFT);
        Label monitorLabel = new Label("Monitor:");
        monitorLabel.setTextFill(Color.WHITE);
        monitorRow.getChildren().addAll(monitorLabel, monitorCombo);
        HBox.setHgrow(monitorCombo, Priority.ALWAYS);

        // GPU options
        boolean nvidiaAvailable = settings.isNvidiaAvailable();
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

        box.getChildren().addAll(header, monitorRow, defaultRb, driRb, driRow, nvidiaRb);
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
        stopButton.setTooltip(new Tooltip("Stop wallpaper on selected monitor"));
        stopButton.setFocusTraversable(true);
        stopButton.setOnAction(e -> stopOnMonitor());

        buttonRow.getChildren().addAll(applyButton, stopButton);

        bottomBox.getChildren().addAll(statusLabel, buttonRow);
        return bottomBox;
    }

    private void setupKeyboardShortcuts(Node root) {
        final KeyCombination ctrlEnter = new KeyCodeCombination(KeyCode.ENTER, KeyCombination.CONTROL_DOWN);
        root.setOnKeyPressed(event -> {
            if (ctrlEnter.match(event)) {
                applyToMonitor();
                event.consume();
            } else if (event.getCode() == KeyCode.ESCAPE) {
                onBack.run();
                event.consume();
            }
        });
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
                List<CustomProperty<?>> props = PropertyParser.parseFromWppr(wallpaper);
                Platform.runLater(() -> {
                    propertiesContainer.getChildren().clear();
                    if (props.isEmpty()) {
                        Label empty = new Label("No properties found in JSON.");
                        empty.setTextFill(Color.LIGHTGRAY);
                        propertiesContainer.getChildren().add(empty);
                        return;
                    }
                    for (CustomProperty<?> prop : props) {
                        allSettings.add(prop);
                        VBox propBox = createSettingBox(prop.getDescription(), prop.createControl(s -> {
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
        List<GlobalOption> globals = Arrays.asList(
                new VolumeSetting(),
                new FpsSetting(),
                new ScalingSetting(),
                new ClampingSetting(),
                new BooleanFlagSetting("disable-mouse", "Disable Mouse Interaction"),
                new BooleanFlagSetting("disable-parallax", "Disable Parallax"),
                new BooleanFlagSetting("silent", "Mute Audio"),
                new BooleanFlagSetting("noautomute", "Don't Auto-mute"),
                new BooleanFlagSetting("no-audio-processing", "Disable Audio Processing"),
                new BooleanFlagSetting("no-fullscreen-pause", "Don't Pause on Fullscreen"),
                new BooleanFlagSetting("fullscreen-pause-only-active", "Pause Only When Active Fullscreen (Wayland)")
        );

        for (GlobalOption opt : globals) {
            allSettings.add(opt);
            Node control = opt.createControl(s -> statusLabel.setText("Global option changed (not yet applied)"));
            VBox optBox = createSettingBox(opt.getDescription(), control);
            globalContainer.getChildren().add(optBox);
        }
    }

    private VBox createSettingBox(String description, Node control) {
        VBox box = new VBox(5);
        box.setPadding(new Insets(10));
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 5;");
        box.setFocusTraversable(false);

        Label descLabel = new Label(description);
        descLabel.setTextFill(Color.LIGHTGRAY);
        descLabel.setStyle("-fx-font-size: 14;");
        descLabel.setFocusTraversable(false);

        box.getChildren().addAll(descLabel, control);
        return box;
    }

    private void applyToMonitor() {
        String monitor = monitorCombo.getValue();
        if (monitor == null || monitor.trim().isEmpty()) {
            statusLabel.setText("Please select a monitor.");
            return;
        }

        GpuConfig gpuConfig = new GpuConfig();
        if (defaultRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.DEFAULT);
        } else if (driRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.DRI_PRIME);
            gpuConfig.setDriPrimeNumber(driSpinner.getValue());
        } else if (nvidiaRb.isSelected()) {
            gpuConfig.setMode(GpuConfig.Mode.NVIDIA);
        }

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
        executor.submit(() -> {
            try {
                wallpaperManager.apply(monitor.trim(), wallpaper.getId(), allSettings,
                        gpuConfig, screenshotPath, screenshotDelay);
                Platform.runLater(() -> statusLabel.setText("Applied to " + monitor));
            } catch (Exception e) {
                Platform.runLater(() -> statusLabel.setText("Error: " + e.getMessage()));
            }
        });
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

        // Load default screenshot path from global settings
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
