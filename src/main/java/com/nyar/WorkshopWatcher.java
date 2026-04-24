package com.nyar;

import java.io.IOException;
import java.nio.file.*;
import java.util.List;

public class WorkshopWatcher {
    private final WpprLib lib;
    private final Runnable onChange;
    private WatchService watchService;
    private final Path workshopPath;

    public WorkshopWatcher(WpprLib lib, Path workshopPath, Runnable onChange) throws IOException {
        this.lib = lib;
        this.onChange = onChange;
        this.watchService = FileSystems.getDefault().newWatchService();
        this.workshopPath = workshopPath;

        workshopPath.register(watchService,
                StandardWatchEventKinds.ENTRY_CREATE,
                StandardWatchEventKinds.ENTRY_DELETE);
    }

    public void startWatching() {
        Thread watcher = new Thread(() -> {
            try {
                while (true) {
                    WatchKey key = watchService.take();

                    boolean changed = false;
                    for (WatchEvent<?> event : key.pollEvents()) {
                        if (event.kind() != StandardWatchEventKinds.OVERFLOW) {
                            changed = true;
                        }
                    }

                    if (changed) {
                        // Refresh library
                        List<Wppr> newWallpapers = WpprRtrv.scanWorkshop(workshopPath);
                        lib.clear();
                        lib.addAll(newWallpapers);
                        // Notify UI
                        if (onChange != null) {
                            onChange.run();
                        }
                    }

                    key.reset();
                }
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            } catch (Exception e) {
                e.printStackTrace();
            }
        });
        watcher.setDaemon(true);
        watcher.start();
    }

    public void stopWatching() {
        try {
            if (watchService != null) {
                watchService.close();
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}