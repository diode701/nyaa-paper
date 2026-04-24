package com.nyar.launcher;

import com.nyar.config.Settings;
import com.nyar.config.SettingsManager;
import javafx.animation.*;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.layout.*;
import javafx.scene.paint.Color;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import javafx.scene.text.Text;
import javafx.stage.Stage;
import javafx.util.Duration;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class NyaaLauncher extends Application {

    private Stage primaryStage;
    private Scene setupScene;
    private boolean isGreen = false;
    // Configuration values
    private int heapSizeMB = 512;
    private final List<String> detectedMonitors = new ArrayList<>();
    private final List<String> selectedMonitors = new ArrayList<>();

    // Paths
    private String steamPath;
    private String pywalPath;
    private String screenshotPath;

    // UI Components
    private ListView<String> monitorListView;
    private TextField customMonitorField;
    private Slider memorySlider;
    private Label memoryLabel;
    private Label statusLabel;
    private TextField steamField;
    private TextField pywalField;
    private TextField screenshotField;

    private final ExecutorService executor = Executors.newSingleThreadExecutor();


    private final int size = 300;

    private static final Path CONFIG_FILE = Paths.get(
            System.getenv().getOrDefault("XDG_CONFIG_HOME",
                    System.getProperty("user.home") + "/.config"),
            "NyaaPaper", "settings.json");

    @Override
    public void start(Stage stage) {
        this.primaryStage = stage;
        primaryStage.setTitle("Nyaa Paper Launcher");

        boolean configExists = Files.exists(CONFIG_FILE);
        boolean forceSetup = "true".equals(System.getProperty("nyaa.forceSetup"));

        if (configExists && !forceSetup) {
            showIntroWithOptions();
        } else {
            showIntroThenSetup();
        }

        primaryStage.show();

        isGreen = detectNvidia();
    }

    // ------------------------------------------------------------------------
    // Intro when config exists → "Launch" and "Setup" options
    // ------------------------------------------------------------------------
    private void showIntroWithOptions() {
        // Load existing settings to populate fields if user clicks Setup
        Settings existing = SettingsManager.load();
        steamPath = existing.getSteamPath();
        pywalPath = existing.getPywalPath();
        screenshotPath = existing.getScreenshotPath();
        selectedMonitors.clear();
        selectedMonitors.addAll(existing.getMonitors());

        StackPane root = new StackPane();
        root.setStyle("-fx-background-color: #1e1e1e;");

        VBox content = new VBox(30);
        content.setAlignment(Pos.CENTER);
        content.setPadding(new Insets(20));

        ImageView catView = new ImageView();
        try {
            catView.setImage(new Image(getClass().getResourceAsStream("/icon.png")));
            catView.setFitWidth(size);
            catView.setPreserveRatio(true);
        } catch (Exception ignored) {}

        Text title = new Text("Nyaa Paper");
        title.setFont(Font.font("SansSerif", FontWeight.BOLD, 48));
        title.setFill(Color.WHITE);

        Text subtitle = new Text("Configuration found.");
        subtitle.setFont(Font.font(16));
        subtitle.setFill(Color.LIGHTGRAY);

        HBox buttonRow = new HBox(20);
        buttonRow.setAlignment(Pos.CENTER);

        Button launchButton = new Button("▶ Launch");
        launchButton.setStyle("-fx-background-color: #4a90e2; -fx-text-fill: white; -fx-font-weight: bold; -fx-font-size: 16;");
        launchButton.setPrefWidth(150);
        launchButton.setOnAction(e -> launchMainApp());
        launchButton.setFocusTraversable(true);

        Button setupButton = new Button("⚙️ Setup");
        setupButton.setStyle("-fx-background-color: #555; -fx-text-fill: white; -fx-font-size: 14;");
        setupButton.setPrefWidth(120);
        setupButton.setOnAction(e -> buildSetupUI());
        setupButton.setFocusTraversable(true);

        Button quitButton = new Button("✖ Quit");
        quitButton.setStyle("-fx-background-color: #a00; -fx-text-fill: white; -fx-font-size: 14;");
        quitButton.setPrefWidth(100);
        quitButton.setOnAction(e -> Platform.exit());
        quitButton.setFocusTraversable(true);

        buttonRow.getChildren().addAll(launchButton, setupButton, quitButton);

        content.getChildren().addAll(catView, title, subtitle, buttonRow);
        root.getChildren().add(content);

        Scene scene = new Scene(root, 800, 600);
        applyStylesheet(scene);
        primaryStage.setScene(scene);

        Platform.runLater(launchButton::requestFocus);

        scene.setOnKeyPressed(e -> {
            switch (e.getCode()) {
                case ENTER -> launchMainApp();
                case ESCAPE -> Platform.exit();
            }
        });
    }

    // ------------------------------------------------------------------------
    // Intro → Setup (first run)
    // ------------------------------------------------------------------------
    private void showIntroThenSetup() {
        // Load defaults (or empty) – SettingsManager.load() will create defaults if file missing
        Settings defaults = SettingsManager.load();
        steamPath = defaults.getSteamPath();
        pywalPath = defaults.getPywalPath();
        screenshotPath = defaults.getScreenshotPath();
        selectedMonitors.clear();
        selectedMonitors.addAll(defaults.getMonitors());

        StackPane root = new StackPane();
        root.setStyle("-fx-background-color: #1e1e1e;");

        VBox content = new VBox(20);
        content.setAlignment(Pos.CENTER);

        ImageView catView = new ImageView();
        try {
            catView.setImage(new Image(getClass().getResourceAsStream("/icon.png")));
            catView.setFitWidth(size);
            catView.setPreserveRatio(true);
        } catch (Exception ignored) {}

        Text title = new Text("Nyaa Paper");
        title.setFont(Font.font("SansSerif", FontWeight.BOLD, 48));
        title.setFill(Color.WHITE);

        Text subtitle = new Text("First time setup");
        subtitle.setFont(Font.font(16));
        subtitle.setFill(Color.LIGHTGRAY);

        content.getChildren().addAll(catView, title, subtitle);
        root.getChildren().add(content);

        Scene scene = new Scene(root, 800, 600);
        applyStylesheet(scene);
        primaryStage.setScene(scene);

        PauseTransition delay = new PauseTransition(Duration.seconds(2));
        delay.setOnFinished(e -> buildSetupUI());
        delay.play();
    }

    // ------------------------------------------------------------------------
    // Setup UI
    // ------------------------------------------------------------------------
 private void buildSetupUI() {
    // Clear any stale selections from previous runs/saved config
    selectedMonitors.clear();

    BorderPane root = new BorderPane();
    root.setStyle("-fx-background-color: #1e1e1e;");

    HBox topBar = new HBox();
    topBar.setPadding(new Insets(20));
    topBar.setAlignment(Pos.CENTER_LEFT);
    topBar.setStyle("-fx-background-color: #2d2d2d;");

    Button backButton = new Button("← Back");
    backButton.setStyle("-fx-font-size: 14;");
    backButton.setOnAction(e -> {
        if (Files.exists(CONFIG_FILE)) {
            showIntroWithOptions();
        } else {
            showIntroThenSetup();
        }
    });
    backButton.setFocusTraversable(true);

    Label title = new Label("Configure Nyaa Paper");
    title.setTextFill(Color.WHITE);
    title.setFont(Font.font(20));
    HBox.setMargin(title, new Insets(0, 0, 0, 20));

    topBar.getChildren().addAll(backButton, title);
    root.setTop(topBar);

    VBox form = new VBox(20);
    form.setPadding(new Insets(20));
    form.setAlignment(Pos.TOP_CENTER);
    form.getChildren().addAll(
            createMonitorSection(),
            createMemorySection(),
            createPathsSection()
    );

    ScrollPane scroll = new ScrollPane(form);
    scroll.setFitToWidth(true);
    scroll.setStyle("-fx-background: transparent; -fx-background-color: transparent;");
    root.setCenter(scroll);

    HBox bottomBar = createBottomBar();
    statusLabel = new Label("Press Tab to navigate, Ctrl+Enter to save & launch");
    statusLabel.setTextFill(Color.LIGHTGRAY);
    statusLabel.setPadding(new Insets(5, 20, 5, 20));
    VBox bottomContainer = new VBox(statusLabel, bottomBar);
    root.setBottom(bottomContainer);

    setupScene = new Scene(root, 800, 800);
    applyStylesheet(setupScene);
    primaryStage.setScene(setupScene);

    // Detect monitors and populate the lists from hardware
    detectMonitors();
    setupKeyboardShortcuts();

    Platform.runLater(() -> monitorListView.requestFocus());
}
    private VBox createMonitorSection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 10; -fx-padding: 15;");

        Label header = new Label("🖥️ Monitors");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(18));

        monitorListView = new ListView<>();
        monitorListView.setPrefHeight(150);
        monitorListView.getSelectionModel().setSelectionMode(SelectionMode.MULTIPLE);
        monitorListView.setCellFactory(lv -> new ListCell<String>() {
            @Override
            protected void updateItem(String item, boolean empty) {
                super.updateItem(item, empty);
                if (empty || item == null) {
                    setText(null);
                    setGraphic(null);
                } else {
                    CheckBox cb = new CheckBox(item);
                    cb.setTextFill(Color.BLACK);
                    cb.setSelected(selectedMonitors.contains(item));
                    cb.setOnAction(e -> {
                        if (cb.isSelected()) selectedMonitors.add(item);
                        else selectedMonitors.remove(item);
                    });
                    setGraphic(cb);
                }
            }
        });

        HBox customRow = new HBox(10);
        customRow.setAlignment(Pos.CENTER_LEFT);
        customMonitorField = new TextField();
        customMonitorField.setPromptText("Custom monitor (e.g., DP-2)");
        customMonitorField.setPrefWidth(300);
        Button addButton = new Button("Add");
        addButton.setOnAction(e -> addCustomMonitor());
        customRow.getChildren().addAll(customMonitorField, addButton);

        Button detectButton = new Button("Detect Monitors");
        detectButton.setOnAction(e -> detectMonitors());

        box.getChildren().addAll(header, monitorListView, customRow, detectButton);
        return box;
    }

    private VBox createMemorySection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 10; -fx-padding: 15;");

        Label header = new Label("Memory Limit (MB)");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(18));

        memorySlider = new Slider(256, 2048, heapSizeMB);
        memorySlider.setShowTickMarks(true);
        memorySlider.setMajorTickUnit(512);
        memorySlider.setBlockIncrement(64);
        memorySlider.setPrefWidth(400);

        memoryLabel = new Label(heapSizeMB + " MB");
        memoryLabel.setTextFill(Color.WHITE);

        memorySlider.valueProperty().addListener((obs, old, val) -> {
            heapSizeMB = val.intValue();
            memoryLabel.setText(heapSizeMB + " MB");
        });

        HBox sliderRow = new HBox(20);
        sliderRow.setAlignment(Pos.CENTER_LEFT);
        sliderRow.getChildren().addAll(memorySlider, memoryLabel);

        box.getChildren().addAll(header, sliderRow);
        return box;
    }

    private VBox createPathsSection() {
        VBox box = new VBox(10);
        box.setStyle("-fx-background-color: #2a2a2a; -fx-background-radius: 10; -fx-padding: 15;");

        Label header = new Label("📁 Paths");
        header.setTextFill(Color.WHITE);
        header.setFont(Font.font(18));

        // Steam path
        Label steamLabel = new Label("Steam Workshop:");
        steamLabel.setTextFill(Color.LIGHTGRAY);
        steamField = new TextField(steamPath);
        steamField.setPromptText("Path to workshop content");
        steamField.textProperty().addListener((obs, old, val) -> steamPath = val);

        // Pywal path
        Label pywalLabel = new Label("Pywal Cache:");
        pywalLabel.setTextFill(Color.LIGHTGRAY);
        pywalField = new TextField(pywalPath);
        pywalField.setPromptText("Path to wal cache");
        pywalField.textProperty().addListener((obs, old, val) -> pywalPath = val);

        // Screenshot path
        Label screenshotLabel = new Label("Screenshot path");
        screenshotLabel.setTextFill(Color.LIGHTGRAY);
        screenshotField = new TextField(screenshotPath);
        screenshotField.setPromptText("Full path (e.g., /home/user/Pictures/wallpaper.png");
        screenshotField.textProperty().addListener((obs, old, val) -> screenshotPath = val);

        box.getChildren().addAll(header,
                steamLabel, steamField,
                pywalLabel, pywalField,
                screenshotLabel, screenshotField);
        return box;
    }

    private HBox createBottomBar() {
        HBox bar = new HBox(20);
        bar.setPadding(new Insets(15));
        bar.setAlignment(Pos.CENTER_RIGHT);
        bar.setStyle("-fx-background-color: #2d2d2d;");

        Button quitButton = new Button("Quit");
        quitButton.setOnAction(e -> Platform.exit());

        Button launchButton = new Button("Save & Launch");
        launchButton.setStyle("-fx-background-color: #4a90e2; -fx-text-fill: white; -fx-font-weight: bold;");
        launchButton.setOnAction(e -> saveAndLaunch());

        bar.getChildren().addAll(quitButton, launchButton);
        return bar;
    }

    private void addCustomMonitor() {
        String custom = customMonitorField.getText().trim();
        if (!custom.isEmpty() && !selectedMonitors.contains(custom)) {
            selectedMonitors.add(custom);
            refreshMonitorList();
            customMonitorField.clear();
        }
    }

    private void refreshMonitorList() {
        monitorListView.getItems().setAll(detectedMonitors);
        monitorListView.refresh();
    }

    private void detectMonitors() {
        statusLabel.setText("Detecting monitors...");
        executor.submit(() -> {
            List<String> found = new ArrayList<>();
            try {
                Process proc = new ProcessBuilder("xrandr", "--query").start();
                BufferedReader reader = new BufferedReader(new InputStreamReader(proc.getInputStream()));
                String line;
                while ((line = reader.readLine()) != null) {
                    if (line.contains(" connected")) {
                        found.add(line.split(" ")[0]);
                    }
                }
                proc.waitFor();
            } catch (Exception e) {
                found.add("eDP-1"); // fallback
            }

            Platform.runLater(() -> {
                // Replace both lists with currently connected monitors
                detectedMonitors.clear();
                detectedMonitors.addAll(found);
                selectedMonitors.clear();
                selectedMonitors.addAll(found);
                refreshMonitorList();
                statusLabel.setText("Monitors detected. Custom monitors can be added manually.");
            });
        });
    }


    private void setupKeyboardShortcuts() {
        setupScene.setOnKeyPressed(event -> {
            switch (event.getCode()) {
                case ESCAPE -> Platform.exit();
                case ENTER -> {
                    if (event.isControlDown()) saveAndLaunch();
                }
            }
        });
    }

    private void saveAndLaunch() {
        if (selectedMonitors.isEmpty()) {
            statusLabel.setText("Please select at least one monitor.");
            return;
        }

        Settings settings = new Settings();
        settings.setMonitorConfig(String.join(",", selectedMonitors));
        settings.setSteamPath(steamPath);
        settings.setPywalPath(pywalPath);
        settings.setScreenshotPath(screenshotPath);
        settings.setNvidiaAvailable(detectNvidia());

        try {
            Files.createDirectories(CONFIG_FILE.getParent());
            SettingsManager.save(settings);
        } catch (IOException e) {
            statusLabel.setText("Failed to save config: " + e.getMessage());
            return;
        }

        statusLabel.setText("Saved. Launching Nyaa Paper...");
        PauseTransition pause = new PauseTransition(Duration.seconds(1));
        pause.setOnFinished(e -> launchMainApp());
        pause.play();
    }

    // ------------------------------------------------------------------------
    // Launch the main application JAR (second process)
    // ------------------------------------------------------------------------
    private void launchMainApp() {
        try {
            Path launcherLocation = Paths.get(NyaaLauncher.class.getProtectionDomain()
                    .getCodeSource().getLocation().toURI());

            Path jarDir;
            if (Files.isDirectory(launcherLocation)) {
                jarDir = Paths.get("build/libs").toAbsolutePath();
            } else {
                jarDir = launcherLocation.getParent();
            }

            Path appJar = null;
            try (DirectoryStream<Path> stream = Files.newDirectoryStream(jarDir, "nyaa-paper-app*.jar")) {
                for (Path p : stream) {
                    appJar = p;
                    break;
                }
            }

            if (appJar == null) {
                Path cwd = Paths.get("").toAbsolutePath();
                try (DirectoryStream<Path> stream = Files.newDirectoryStream(cwd, "nyaa-paper-app*.jar")) {
                    for (Path p : stream) {
                        appJar = p;
                        break;
                    }
                }
            }

            if (appJar == null || !Files.exists(appJar)) {
                statusLabel.setText("Main app JAR not found. Run './gradlew buildAll' first.");
                return;
            }

            String javaBin = Paths.get(System.getProperty("java.home"), "bin", "java").toString();
            List<String> command = new ArrayList<>();
            command.add(javaBin);
            command.add("-Xmx" + heapSizeMB + "m");
            command.add("-jar");
            command.add(appJar.toString());

            new ProcessBuilder(command).inheritIO().start();
            Platform.exit();
        } catch (Exception ex) {
            statusLabel.setText("Launch failed: " + ex.getMessage());
            ex.printStackTrace();
        }
    }

    private void applyStylesheet(Scene scene) {
        try {
            scene.getStylesheets().add(getClass().getResource("/dark-theme.css").toExternalForm());
        } catch (Exception ignored) {}
    }
    private boolean detectNvidia() {
        try {
            Process proc = new ProcessBuilder("nvidia-smi").start();
            return proc.waitFor() == 0;
        } catch (Exception e) {
            return Files.exists(Paths.get("/sys/module/nvidia"));
        }
    }
    public static void main(String[] args) {
        if (args.length > 0 && "--setup".equals(args[0])) {
            System.setProperty("nyaa.forceSetup", "true");
        }
        launch(args);
    }
}