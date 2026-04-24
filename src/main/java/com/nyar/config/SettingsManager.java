package com.nyar.config;

import org.json.JSONObject;
import org.json.JSONTokener;
import java.io.*;
import java.nio.file.*;

/**
 * Handles loading and saving Settings to the user's config file.
 */
public class SettingsManager {
    private static final Path CONFIG_DIR = Paths.get(
            System.getenv().getOrDefault("XDG_CONFIG_HOME",
                    System.getProperty("user.home") + "/.config"),
            "NyaaPaper");
    private static final Path CONFIG_FILE = CONFIG_DIR.resolve("settings.json");

    public static Settings load() {
        if (Files.exists(CONFIG_FILE)) {
            try (Reader reader = Files.newBufferedReader(CONFIG_FILE)) {
                JSONObject obj = new JSONObject(new JSONTokener(reader));
                return Settings.fromJSON(obj);
            } catch (Exception e) {
                System.err.println("Failed to load settings, using defaults: " + e.getMessage());
            }
        }
        Settings defaults = new Settings();
        save(defaults);  // Create the file for next time
        return defaults;
    }

    public static void save(Settings settings) {
        try {
            Files.createDirectories(CONFIG_DIR);
            try (Writer writer = Files.newBufferedWriter(CONFIG_FILE)) {
                writer.write(settings.toJSON().toString(2));
            }
        } catch (IOException e) {
            System.err.println("Failed to save settings: " + e.getMessage());
        }
    }
}
