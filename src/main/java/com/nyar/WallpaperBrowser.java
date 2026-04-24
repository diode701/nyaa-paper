package com.nyar;

import com.nyar.config.Settings;
import com.nyar.config.SettingsManager;
import com.nyar.wallpaper.WallpaperManager;
import dorkbox.systemTray.SystemTray;
import dorkbox.systemTray.MenuItem;

import javafx.application.Application;
import javafx.application.Platform;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.CacheHint;
import javafx.scene.Scene;
import javafx.scene.control.Label;
import javafx.scene.effect.DropShadow;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.BorderPane;
import javafx.scene.layout.GridPane;
import javafx.scene.layout.StackPane;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import javafx.stage.Stage;
import javafx.animation.FadeTransition;
import javafx.animation.PauseTransition;
import javafx.util.Duration;

import java.io.IOException;
import java.net.URL;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.concurrent.*;

public class WallpaperBrowser extends Application {

    // ---------- Backend ----------
    private WpprLib library;
    private WorkshopWatcher watcher;

    // ---------- UI Components ----------
    private GridPane grid;
    private Label statusLabel;
    private BorderPane root;
    private VBox gridContainer;

    // ---------- Dynamic grid dimensions ----------
    private int gridCols = 4;
    private int gridRows = 5;
    private int pageSize = gridCols * gridRows;

    // ---------- Page state ----------
    private List<Wppr> currentPage = Collections.emptyList();
    private List<Wppr> previousPage = Collections.emptyList();
    private int currentPageIndex = 0;
    private int selectedIndex = 0;
    private String selectedId = null;

    // ---------- Thumbnail handling ----------
    private ExecutorService thumbnailLoader;
    private final Map<String, Future<?>> loadingTasks = new ConcurrentHashMap<>();
    private final Map<String, Future<?>> preloadTasks = new ConcurrentHashMap<>();
    private final Map<String, Image> imageCache = new ConcurrentHashMap<>();
    private final Map<StackPane, FadeTransition> cellTransitions = new IdentityHashMap<>();

    // ---------- UI Helpers ----------
    private final PauseTransition resizeDebouncer = new PauseTransition(Duration.millis(150));
    private ScheduledExecutorService scheduler;

    // ---------- System Tray ----------
    private SystemTray systemTray;

    // ---------- Constants ----------
    private static final int THUMB_SIZE = 180;
    private static final int HGAP = 15;
    private static final int VGAP = 15;

    private WallpaperPropertiesView currentPropertiesView = null;

    @Override
    public void start(Stage stage) throws IOException {
        Platform.setImplicitExit(false);

        // Create daemon thread pools
        thumbnailLoader = Executors.newFixedThreadPool(2, r -> {
            Thread t = new Thread(r);
            t.setDaemon(true);
            return t;
        });
        scheduler = Executors.newSingleThreadScheduledExecutor(r -> {
            Thread t = new Thread(r);
            t.setDaemon(true);
            return t;
        });

        // Load settings and build library
        Settings config = SettingsManager.load();
        Builder builder = new Builder(config).scanWorkshop();
        library = builder.build();
        watcher = builder.buildWatcher(library, this::onLibraryChange);
// SHUTDOWN HOOK - ensures wallpapers are killed even on SIGTERM
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            System.out.println("Shutdown hook: stopping all wallpaper processes");
            WallpaperManager.getInstance().stopAll();
        }));
        initUI(stage);
        recomputeGrid();
        loadPage(0);
        watcher.startWatching();

        stage.setOnCloseRequest(e -> {
            e.consume();
            collapseToTray(stage);
        });
    }

    private void initUI(Stage stage) {
        root = new BorderPane();
        root.setStyle("-fx-background-color: #1e1e1e;");

        gridContainer = new VBox();
        gridContainer.setAlignment(Pos.CENTER);
        gridContainer.setFillWidth(true);

        grid = new GridPane();
        grid.setHgap(HGAP);
        grid.setVgap(VGAP);
        grid.setAlignment(Pos.CENTER);
        grid.setPadding(new Insets(20));

        gridContainer.getChildren().add(grid);
        root.setCenter(gridContainer);

        statusLabel = new Label();
        statusLabel.setMaxWidth(Double.MAX_VALUE);
        statusLabel.setAlignment(Pos.CENTER_LEFT);
        statusLabel.getStyleClass().add("status-bar");
        root.setBottom(statusLabel);

        Scene scene = new Scene(root, 1000, 800);
        URL cssUrl = getClass().getResource("/dark-theme.css");
        if (cssUrl != null) {
            scene.getStylesheets().add(cssUrl.toExternalForm());
        }
        setupKeyHandlers(scene);

        resizeDebouncer.setOnFinished(e -> {
            recomputeGrid();
            loadPage(currentPageIndex);
        });

        stage.widthProperty().addListener((obs, oldVal, newVal) -> resizeDebouncer.playFromStart());
        stage.heightProperty().addListener((obs, oldVal, newVal) -> resizeDebouncer.playFromStart());

        stage.setTitle("Wallpaper Browser");
        stage.setScene(scene);
        stage.show();
    }

    // ----------------------------------------------------------------------
    // System Tray
    // ----------------------------------------------------------------------
    private void collapseToTray(Stage stage) {
        if (systemTray == null) {
            SystemTray.DEBUG = true;
            systemTray = SystemTray.get();

            if (systemTray == null) {
                System.out.println("SystemTray is not supported on this OS/DE.");
                suspendToTray(stage);
                return;
            }

            URL iconUrl = getClass().getResource("/icon.png");
            if (iconUrl != null) {
                systemTray.setImage(iconUrl);
            } else {
                System.out.println("Warning: /icon.png not found in resources.");
            }

            systemTray.setTooltip("Wallpaper Browser");

            systemTray.getMenu().add(new MenuItem("Restore", e -> {
                Platform.runLater(() -> restoreFromTray(stage));
            }));

            systemTray.getMenu().add(new MenuItem("Exit", e -> {
                hardExit();
            }));
        }

        suspendToTray(stage);
    }

    private void suspendToTray(Stage stage) {
        stage.hide();

        // Cancel all pending tasks
        loadingTasks.values().forEach(f -> f.cancel(true));
        loadingTasks.clear();
        preloadTasks.values().forEach(f -> f.cancel(true));
        preloadTasks.clear();

        // Clear all cell images and transitions
        for (int i = 0; i < gridCols * gridRows; i++) {
            StackPane cell = getCell(i);
            FadeTransition ft = cellTransitions.remove(cell);
            if (ft != null) ft.stop();
            cell.getChildren().clear();
        }
        cellTransitions.clear();

        imageCache.clear();
        System.out.println("[App] Suspended to tray.");
    }

    private void restoreFromTray(Stage stage) {
        // Recreate executor if it was shut down (not needed because daemon threads still alive)
        if (thumbnailLoader.isShutdown()) {
            thumbnailLoader = Executors.newFixedThreadPool(2, r -> {
                Thread t = new Thread(r);
                t.setDaemon(true);
                return t;
            });
        }
        loadPage(currentPageIndex);
        stage.show();
        System.out.println("[App] Restored from tray.");
    }

    private void hardExit() {
        // Cancel all background work
        if (thumbnailLoader != null && !thumbnailLoader.isShutdown()) {
            thumbnailLoader.shutdownNow();
        }
        if (scheduler != null && !scheduler.isShutdown()) {
            scheduler.shutdownNow();
        }

        if (currentPropertiesView != null) {
            currentPropertiesView.shutdown();
        }
        loadingTasks.values().forEach(f -> f.cancel(true));
        preloadTasks.values().forEach(f -> f.cancel(true));
        if (watcher != null) watcher.stopWatching();
        imageCache.clear();
        cellTransitions.clear();

        // Stop all wallpaper processes in a background thread (to avoid UI freeze)
        Thread stopper = new Thread(() -> {
            System.out.println("[Exit] Stopping all wallpaper instances...");
            WallpaperManager.getInstance().stopAll();
            System.out.println("[Exit] All wallpapers stopped.");
        });
        stopper.setDaemon(false);
        stopper.start();

        try {
            // Wait up to 5 seconds for all wallpapers to terminate
            stopper.join(5000);
        } catch (InterruptedException ignored) {}

        if (systemTray != null) {
            systemTray.shutdown();
        }

        Platform.exit();
        System.exit(0);
    }
    // ----------------------------------------------------------------------
    // Grid Management
    // ----------------------------------------------------------------------
    private void recomputeGrid() {
        double usableWidth = root.getWidth() - 40;
        double usableHeight = root.getHeight() - 80;

        if (usableWidth < THUMB_SIZE || usableHeight < THUMB_SIZE) {
            gridCols = 1;
            gridRows = 1;
        } else {
            int maxCols = (int) ((usableWidth + HGAP) / (THUMB_SIZE + HGAP));
            int maxRows = (int) ((usableHeight + VGAP) / (THUMB_SIZE + VGAP));
            gridCols = Math.max(1, maxCols);
            gridRows = Math.max(1, maxRows);
        }
        pageSize = gridCols * gridRows;
        rebuildGrid();
    }

    private void rebuildGrid() {
        grid.getChildren().clear();
        cellTransitions.clear();   // 🔥 FIX: prevent memory leak from stale transitions
        for (int row = 0; row < gridRows; row++) {
            for (int col = 0; col < gridCols; col++) {
                grid.add(createEmptyCell(), col, row);
            }
        }
    }

    private StackPane createEmptyCell() {
        StackPane cell = new StackPane();
        cell.setPrefSize(THUMB_SIZE, THUMB_SIZE);
        cell.setMinSize(THUMB_SIZE, THUMB_SIZE);
        cell.setMaxSize(THUMB_SIZE, THUMB_SIZE);
        cell.getStyleClass().add("grid-cell");

        DropShadow shadow = new DropShadow();
        shadow.setColor(Color.rgb(0, 0, 0, 0.5));
        shadow.setRadius(10);
        shadow.setOffsetX(2);
        shadow.setOffsetY(2);
        cell.setEffect(shadow);

        cell.setCache(true);
        cell.setCacheShape(true);
        cell.setCacheHint(CacheHint.SPEED);

        return cell;
    }

    private StackPane getCell(int index) {
        int row = index / gridCols;
        int col = index % gridCols;
        return (StackPane) grid.getChildren().get(row * gridCols + col);
    }

    // ----------------------------------------------------------------------
    // Keyboard Navigation
    // ----------------------------------------------------------------------
    private void setupKeyHandlers(Scene scene) {
        scene.setOnKeyPressed(event -> {
            KeyCode code = event.getCode();
            switch (code) {
                case UP:    moveSelection(-gridCols); break;
                case DOWN:  moveSelection(gridCols); break;
                case LEFT:  moveSelection(-1); break;
                case RIGHT: moveSelection(1); break;
                case PAGE_UP:
                case N:     if (currentPageIndex > 0) loadPage(currentPageIndex - 1); break;
                case PAGE_DOWN:
                case P:     if (currentPageIndex < totalPages() - 1) loadPage(currentPageIndex + 1); break;
                case ENTER: selectCurrent(); break;
                case Q:     hardExit(); break;
                default: break;
            }
        });
    }

    private void moveSelection(int delta) {
        int newIndex = selectedIndex + delta;

        // Moving UP across pages
        if (delta == -gridCols && selectedIndex < gridCols && currentPageIndex > 0) {
            int targetCol = selectedIndex % gridCols; // Capture BEFORE loadPage modifies selectedIndex
            loadPage(currentPageIndex - 1);

            Platform.runLater(() -> {
                int newSel = (gridRows - 1) * gridCols + targetCol;
                if (newSel >= currentPage.size()) newSel = currentPage.size() - 1; // Safety bounds

                if (newSel >= 0) {
                    // Pass the current 'selectedIndex' (which loadPage just set to 0) to clear it
                    updateSelection(selectedIndex, newSel);
                    selectedIndex = newSel;
                    selectedId = currentPage.get(selectedIndex).getId();
                    updateStatusBar();
                }
            });
            return;
        }

        // Moving DOWN across pages
        if (delta == gridCols && selectedIndex >= (gridRows - 1) * gridCols && currentPageIndex < totalPages() - 1) {
            int targetCol = selectedIndex % gridCols; // Capture BEFORE loadPage modifies selectedIndex
            loadPage(currentPageIndex + 1);

            Platform.runLater(() -> {
                int newSel = targetCol;
                if (newSel >= currentPage.size()) newSel = currentPage.size() - 1; // Safety bounds

                if (newSel >= 0) {
                    // Pass the current 'selectedIndex' (which loadPage just set to 0) to clear it
                    updateSelection(selectedIndex, newSel);
                    selectedIndex = newSel;
                    selectedId = currentPage.get(selectedIndex).getId();
                    updateStatusBar();
                }
            });
            return;
        }

        // Standard movement on the same page
        if (newIndex >= 0 && newIndex < currentPage.size()) {
            updateSelection(selectedIndex, newIndex);
            selectedIndex = newIndex;
            selectedId = currentPage.get(selectedIndex).getId();
            updateStatusBar();
        }
    }

    private void updateSelection(int oldIdx, int newIdx) {
        if (oldIdx >= 0 && oldIdx < currentPage.size()) {
            getCell(oldIdx).getStyleClass().remove("selected");
        }
        if (newIdx >= 0 && newIdx < currentPage.size()) {
            StackPane cell = getCell(newIdx);
            // Check if it already has the class to prevent duplicates building up
            if (!cell.getStyleClass().contains("selected")) {
                cell.getStyleClass().add("selected");
            }
        }
    }


    private void updateStatusBar() {
        int total = totalPages();
        String pageInfo = String.format("Page %d/%d", currentPageIndex + 1, total);
        String selInfo = "";
        if (!currentPage.isEmpty() && selectedIndex < currentPage.size()) {
            selInfo = " | " + currentPage.get(selectedIndex).getTitle();
        }
        statusLabel.setText(pageInfo + selInfo + " | ←↑↓→ move | N/P page | Enter select | Q quit");
    }

    private int totalPages() {
        return (int) Math.ceil((double) library.size() / pageSize);
    }

    // ----------------------------------------------------------------------
    // Navigation Methods
    // ----------------------------------------------------------------------
    private void selectCurrent() {
        if (selectedIndex < currentPage.size()) {
            Wppr wp = currentPage.get(selectedIndex);
            showProperties(wp);
        }
    }

   private void showProperties(Wppr wp) {
    // Shutdown any existing properties view first
    if (currentPropertiesView != null) {
        currentPropertiesView.shutdown();
    }
    currentPropertiesView = new WallpaperPropertiesView(wp, this::showGrid);
    root.setCenter(currentPropertiesView.getView());
}

private void showGrid() {
    if (currentPropertiesView != null) {
        currentPropertiesView.shutdown();
        currentPropertiesView = null;
    }
    root.setCenter(gridContainer);
    loadPage(currentPageIndex);
}

    // ----------------------------------------------------------------------
    // Page Loading
    // ----------------------------------------------------------------------
    private void loadPage(int page) {
        // Cancel all ongoing loading tasks for the previous page
        loadingTasks.values().forEach(f -> f.cancel(true));
        loadingTasks.clear();
        preloadTasks.values().forEach(f -> f.cancel(true));
        preloadTasks.clear();

        int total = totalPages();
        if (total == 0) {
            previousPage = currentPage;
            currentPage = Collections.emptyList();
            currentPageIndex = 0;
        } else {
            previousPage = currentPage;
            currentPageIndex = Math.max(0, Math.min(page, total - 1));
            currentPage = library.getPage(currentPageIndex, pageSize);
        }

        // Clear UI cells
        for (int i = 0; i < gridCols * gridRows; i++) {
            StackPane cell = getCell(i);
            FadeTransition ft = cellTransitions.remove(cell);
            if (ft != null) ft.stop();
            cell.getChildren().clear();
            cell.getStyleClass().remove("selected");
            cell.setUserData(null);
        }

        // Load images for current page
        for (int i = 0; i < currentPage.size(); i++) {
            Wppr wp = currentPage.get(i);
            StackPane cell = getCell(i);
            cell.setUserData(wp.getId());

            Image cached = imageCache.get(wp.getId());
            if (cached != null && !cached.isError()) {
                setCellImage(cell, cached);
            } else {
                Label placeholder = new Label(shortTitle(wp.getTitle()));
                placeholder.setTextFill(Color.LIGHTGRAY);
                placeholder.setWrapText(true);
                placeholder.setMaxWidth(THUMB_SIZE - 10);
                placeholder.setAlignment(Pos.CENTER);
                cell.getChildren().add(placeholder);

                Future<?> future = thumbnailLoader.submit(() -> loadImage(wp, cell));
                loadingTasks.put(wp.getId(), future);
            }
        }

        preloadAdjacentPages();
        trimCache();

        // Restore selection
        if (selectedId != null) {
            int newIdx = findIndexById(selectedId);
            selectedIndex = (newIdx >= 0) ? newIdx : 0;
        } else {
            selectedIndex = 0;
        }
        if (!currentPage.isEmpty()) {
            selectedId = currentPage.get(selectedIndex).getId();
            updateSelection(-1, selectedIndex);
        }
        updateStatusBar();
    }

    private int findIndexById(String id) {
        for (int i = 0; i < currentPage.size(); i++) {
            if (currentPage.get(i).getId().equals(id)) return i;
        }
        return -1;
    }

    private void preloadAdjacentPages() {
        int total = totalPages();
        if (total == 0) return;

        int[] pagesToLoad = { currentPageIndex - 1, currentPageIndex + 1 };
        for (int p : pagesToLoad) {
            if (p >= 0 && p < total && p != currentPageIndex) {
                List<Wppr> pageWps = library.getPage(p, pageSize);
                for (Wppr wp : pageWps) {
                    if (!imageCache.containsKey(wp.getId()) && !preloadTasks.containsKey(wp.getId())) {
                        Future<?> f = thumbnailLoader.submit(() -> {
                            try {
                                String path = findPreviewPath(wp);
                                if (path != null && !Thread.currentThread().isInterrupted()) {
                                    Image img = new Image(Path.of(path).toUri().toString(),
                                            THUMB_SIZE, THUMB_SIZE, true, true, true);
                                    if (!img.isError() && !Thread.currentThread().isInterrupted()) {
                                        imageCache.put(wp.getId(), img);
                                    }
                                }
                            } finally {
                                preloadTasks.remove(wp.getId());
                            }
                        });
                        preloadTasks.put(wp.getId(), f);
                    }
                }
            }
        }
    }

    private void trimCache() {
        int total = totalPages();
        if (total == 0) {
            imageCache.clear();
            return;
        }

        Set<String> keepIds = new HashSet<>();
        for (Wppr wp : currentPage) keepIds.add(wp.getId());

        if (currentPageIndex > 0) {
            List<Wppr> prev = library.getPage(currentPageIndex - 1, pageSize);
            for (Wppr wp : prev) keepIds.add(wp.getId());
        }
        if (currentPageIndex < total - 1) {
            List<Wppr> next = library.getPage(currentPageIndex + 1, pageSize);
            for (Wppr wp : next) keepIds.add(wp.getId());
        }

        // Cancel preload tasks for images we are about to evict
        Set<String> toRemove = new HashSet<>(imageCache.keySet());
        toRemove.removeAll(keepIds);
        for (String id : toRemove) {
            Future<?> f = preloadTasks.remove(id);
            if (f != null) f.cancel(true);
            loadingTasks.remove(id);
        }
        imageCache.keySet().retainAll(keepIds);
    }

    private void loadImage(Wppr wp, StackPane cell) {
        if (Thread.currentThread().isInterrupted()) return;

        String path = findPreviewPath(wp);
        if (path == null) return;

        Image img = new Image(Path.of(path).toUri().toString(),
                THUMB_SIZE, THUMB_SIZE, true, true, true);

        if (img.isError() || Thread.currentThread().isInterrupted()) return;

        imageCache.put(wp.getId(), img);

        Platform.runLater(() -> {
            if (wp.getId().equals(cell.getUserData())) {
                setCellImage(cell, img);
            }
            loadingTasks.remove(wp.getId());
        });
    }

    private void setCellImage(StackPane cell, Image img) {
        FadeTransition old = cellTransitions.remove(cell);
        if (old != null) old.stop();

        cell.getChildren().clear();
        ImageView iv = new ImageView(img);
        iv.setPreserveRatio(true);
        iv.setFitWidth(THUMB_SIZE - 10);
        iv.setFitHeight(THUMB_SIZE - 10);

        iv.setCache(true);
        iv.setCacheHint(CacheHint.SPEED);

        cell.getChildren().add(iv);

        FadeTransition ft = new FadeTransition(Duration.millis(300), iv);
        ft.setFromValue(0);
        ft.setToValue(1);
        cellTransitions.put(cell, ft);
        ft.setOnFinished(e -> cellTransitions.remove(cell));
        ft.play();
    }

    private String shortTitle(String title) {
        if (title.length() > 20) return title.substring(0, 18) + "…";
        return title;
    }

    private String findPreviewPath(Wppr wp) {
        String base = wp.getPreviewPath();
        Path jpg = Paths.get(base);
        if (Files.exists(jpg)) return jpg.toString();
        Path gif = Paths.get(base.replace(".jpg", ".gif"));
        if (Files.exists(gif)) return gif.toString();
        return null;
    }

    private void onLibraryChange() {
        Platform.runLater(() -> {
            library.refresh();
            loadPage(currentPageIndex);
        });
    }

    public static void main(String[] args) {
        launch(args);
    }
}