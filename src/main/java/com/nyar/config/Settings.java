package com.nyar.config;

import org.json.JSONObject;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

/**
 * User configuration for Nyaa Paper.
 */
public class Settings {
    private boolean illogicalImpulse = false;
    private int maxRam = 512;
    private String steamPath = System.getProperty("user.home") +
            "/.local/share/Steam/steamapps/workshop/content/431960/";
    private String pywalPath = System.getProperty("user.home") + "/.cache/wal/";
    private String monitorConfig = "eDP-1";
    private String screenshotPath = System.getProperty("user.home") + "/Pictures/";
    private boolean nvidiaAvailable = false;
    private boolean persistOnStart = true;
    private boolean autoStartOnBoot = false;

    public Settings() {}

    // Getters and setters
    public boolean isIllogicalImpulse() { return illogicalImpulse; }
    public void setIllogicalImpulse(boolean illogicalImpulse) { this.illogicalImpulse = illogicalImpulse; }

    public int getMaxRam() { return maxRam; }
    public void setMaxRam(int maxRam) { this.maxRam = maxRam; }

    public String getSteamPath() { return steamPath; }
    public void setSteamPath(String steamPath) { this.steamPath = steamPath; }

    public String getPywalPath() { return pywalPath; }
    public void setPywalPath(String pywalPath) { this.pywalPath = pywalPath; }

    public String getMonitorConfig() { return monitorConfig; }
    public void setMonitorConfig(String monitorConfig) { this.monitorConfig = monitorConfig; }

    public String getScreenshotPath() { return screenshotPath; }
    public void setScreenshotPath(String screenshotPath) { this.screenshotPath = screenshotPath; }

    public boolean isNvidiaAvailable() { return nvidiaAvailable; }
    public void setNvidiaAvailable(boolean nvidiaAvailable) { this.nvidiaAvailable = nvidiaAvailable; }

    public boolean isPersistOnStart() { return persistOnStart; }
    public void setPersistOnStart(boolean persistOnStart) { this.persistOnStart = persistOnStart; }

    public boolean isAutoStartOnBoot() { return autoStartOnBoot; }
    public void setAutoStartOnBoot(boolean autoStartOnBoot) { this.autoStartOnBoot = autoStartOnBoot; }

    public List<String> getMonitors() {
        if (monitorConfig == null || monitorConfig.trim().isEmpty()) {
            return Arrays.asList("eDP-1");
        }
        return Arrays.stream(monitorConfig.split(","))
                .map(String::trim)
                .filter(s -> !s.isEmpty())
                .collect(Collectors.toList());
    }

    public JSONObject toJSON() {
        JSONObject obj = new JSONObject();
        obj.put("illogicalImpulse", illogicalImpulse);
        obj.put("maxRam", maxRam);
        obj.put("steamPath", steamPath);
        obj.put("pywalPath", pywalPath);
        obj.put("monitorConfig", monitorConfig);
        obj.put("screenshotPath", screenshotPath);
        obj.put("nvidiaAvailable", nvidiaAvailable);
        obj.put("persistOnStart", persistOnStart);
        obj.put("autoStartOnBoot", autoStartOnBoot);
        return obj;
    }

    public static Settings fromJSON(JSONObject obj) {
        Settings s = new Settings();
        if (obj.has("illogicalImpulse")) s.illogicalImpulse = obj.getBoolean("illogicalImpulse");
        if (obj.has("maxRam")) s.maxRam = obj.getInt("maxRam");
        if (obj.has("steamPath")) s.steamPath = obj.getString("steamPath");
        if (obj.has("pywalPath")) s.pywalPath = obj.getString("pywalPath");
        if (obj.has("monitorConfig")) s.monitorConfig = obj.getString("monitorConfig");
        if (obj.has("screenshotPath")) s.screenshotPath = obj.getString("screenshotPath");
        if (obj.has("nvidiaAvailable")) s.nvidiaAvailable = obj.getBoolean("nvidiaAvailable");
        if (obj.has("persistOnStart")) s.persistOnStart = obj.getBoolean("persistOnStart");
        if (obj.has("autoStartOnBoot")) s.autoStartOnBoot = obj.getBoolean("autoStartOnBoot");
        return s;
    }
}
