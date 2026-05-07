package com.nyar.wallpaper;

import com.nyar.settings.Setting;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.function.BiConsumer;

public class WallpaperManager {
    private static final WallpaperManager INSTANCE = new WallpaperManager();
    private final Map<String, WallpaperInstance> instances = new ConcurrentHashMap<>();
    private BiConsumer<String, Integer> crashListener;

    private WallpaperManager() {}

    public static WallpaperManager getInstance() {
        return INSTANCE;
    }

    public void setCrashListener(BiConsumer<String, Integer> listener) {
        this.crashListener = listener;
    }

    void notifyCrash(String monitor, int exitCode, WallpaperInstance crashed) {
        // Only remove if it's still the same instance (not already replaced)
        if (instances.remove(monitor, crashed)) {
            if (crashListener != null) {
                crashListener.accept(monitor, exitCode);
            }
        }
    }

 public synchronized void apply(String monitor, String wallpaperId, List<Setting> settings,
                               GpuConfig gpuConfig, String screenshotPath, int screenshotDelay) throws IOException {
    WallpaperInstance existing = instances.get(monitor);
    if (existing != null) {
        existing.stop();
    }
    WallpaperInstance instance = new WallpaperInstance(monitor, wallpaperId, settings, gpuConfig, screenshotPath, screenshotDelay);
    instances.put(monitor, instance);
}
    public synchronized void stop(String monitor) {
        WallpaperInstance instance = instances.remove(monitor);
        if (instance != null) {
            instance.stop();
        }
    }

    // In WallpaperManager.java
    public synchronized void stopAll() {
        List<WallpaperInstance> copy = new ArrayList<>(instances.values());
        instances.clear();
        for (WallpaperInstance instance : copy) {
            instance.stop();  // each stops within ~5 seconds max
        }
    }
    public WallpaperInstance getInstance(String monitor) {
        return instances.get(monitor);
    }
}
