package com.nyar.config;

import org.json.JSONObject;
import java.io.IOException;
import java.nio.file.*;
import java.util.Set;

public class WallpaperConfigManager {
    private static final WallpaperConfigManager INSTANCE = new WallpaperConfigManager();
    private final Path baseDir;

    private WallpaperConfigManager() {
        baseDir = Paths.get(System.getProperty("user.home"), ".config", "NyaaPaper", "wallpapers");
    }

    public static WallpaperConfigManager getInstance() {
        return INSTANCE;
    }

    public WallpaperConfig load(String monitor, String wallpaperId) {
        Path file = baseDir.resolve(monitor).resolve(wallpaperId + ".json");
        if (!Files.exists(file)) return null;
        try {
            String raw = Files.readString(file);
            return WallpaperConfig.fromJson(new JSONObject(raw));
        } catch (IOException e) {
            System.out.println("Failed to load config " + file + ": " + e.getMessage());
            // Corrupt file – delete it so we don't keep failing
            try { Files.deleteIfExists(file); } catch (IOException ignored) {}
            return null;
        }
    }

    public void save(WallpaperConfig config) {
        try {
            Path dir = baseDir.resolve(config.getMonitor());
            Files.createDirectories(dir);

            // Remove all previous configs for this monitor – only one active at a time
            if (Files.exists(dir)) {
                try (DirectoryStream<Path> stream = Files.newDirectoryStream(dir, "*.json")) {
                    for (Path old : stream) {
                        Files.deleteIfExists(old);
                    }
                }
            }

            Path file = dir.resolve(config.getWallpaperId() + ".json");
            Files.writeString(file, config.toJson().toString(2));
        } catch (IOException e) {
            System.out.println("Failed to save config: " + e.getMessage());
        }
    }
    public void delete(String monitor, String wallpaperId) {
        try {
            Path dir = baseDir.resolve(monitor);
            Path file = dir.resolve(wallpaperId + ".json");
            Files.deleteIfExists(file);
            // Clean empty monitor dir
            try { Files.deleteIfExists(dir); } catch (IOException ignored) {}
        } catch (IOException e) {
            System.out.println("Failed to delete config: " + e.getMessage());
        }
    }

    public void cleanupStale(Set<String> activeWorkshopIds) {
        if (!Files.exists(baseDir)) return;
        try {
            for (Path monitorDir : Files.newDirectoryStream(baseDir)) {
                if (!Files.isDirectory(monitorDir)) continue;
                for (Path cfgFile : Files.newDirectoryStream(monitorDir, "*.json")) {
                    String name = cfgFile.getFileName().toString();
                    String id = name.substring(0, name.length() - 5); // strip .json
                    if (!activeWorkshopIds.contains(id)) {
                        Files.deleteIfExists(cfgFile);
                    }
                }
                // Remove empty monitor dirs
                try (var stream = Files.newDirectoryStream(monitorDir)) {
                    if (!stream.iterator().hasNext()) {
                        Files.deleteIfExists(monitorDir);
                    }
                }
            }
        } catch (IOException e) {
            System.out.println("Cleanup of stale configs failed: " + e.getMessage());
        }
    }
}
