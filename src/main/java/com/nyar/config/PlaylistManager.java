package com.nyar.config;

import org.json.JSONObject;
import java.io.IOException;
import java.nio.file.*;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class PlaylistManager {
    private static final PlaylistManager INSTANCE = new PlaylistManager();
    private final Path baseDir;

    private PlaylistManager() {
        baseDir = Paths.get(
                System.getenv().getOrDefault("XDG_CONFIG_HOME",
                        System.getProperty("user.home") + "/.config"),
                "NyaaPaper", "playlist");
    }

    public static PlaylistManager getInstance() { return INSTANCE; }

    public Playlist load(String monitor) {
        Path file = baseDir.resolve(monitor + ".json");
        if (!Files.exists(file)) return null;
        try {
            String raw = Files.readString(file);
            return Playlist.fromJson(new JSONObject(raw));
        } catch (IOException e) {
            System.out.println("Failed to load playlist for " + monitor + ": " + e.getMessage());
            try { Files.deleteIfExists(file); } catch (IOException ignored) {}
            return null;
        }
    }

    public void save(String monitor, Playlist playlist) {
        try {
            Files.createDirectories(baseDir);
            Path file = baseDir.resolve(monitor + ".json");
            Files.writeString(file, playlist.toJson().toString(2));
        } catch (IOException e) {
            System.out.println("Failed to save playlist for " + monitor + ": " + e.getMessage());
        }
    }

    public void delete(String monitor) {
        try {
            Path file = baseDir.resolve(monitor + ".json");
            Files.deleteIfExists(file);
        } catch (IOException e) {
            System.out.println("Failed to delete playlist for " + monitor + ": " + e.getMessage());
        }
    }

    public List<String> listMonitors() {
        List<String> monitors = new ArrayList<>();
        if (!Files.exists(baseDir)) return monitors;
        try (Stream<Path> files = Files.list(baseDir)) {
            files.filter(Files::isRegularFile)
                 .filter(f -> f.getFileName().toString().endsWith(".json"))
                 .forEach(f -> {
                     String name = f.getFileName().toString();
                     monitors.add(name.substring(0, name.length() - 5));
                 });
        } catch (IOException e) {
            System.out.println("Failed to list playlist monitors: " + e.getMessage());
        }
        return monitors;
    }
}
