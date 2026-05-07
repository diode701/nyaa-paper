package com.nyar.config;

import org.json.JSONObject;
import java.util.LinkedHashMap;
import java.util.Map;

public class WallpaperConfig {
    public static final int CURRENT_VERSION = 1;

    private int version;
    private String wallpaperId;
    private String monitor;
    private long projectJsonModified;
    private final Map<String, String> settings;

    // GPU / apply state
    private String gpuMode = "DEFAULT";
    private int driPrimeNumber = 1;
    private boolean glThreadedOpt = false;

    public WallpaperConfig() {
        this.version = CURRENT_VERSION;
        this.settings = new LinkedHashMap<>();
    }

    public WallpaperConfig(String wallpaperId, String monitor, long projectJsonModified) {
        this();
        this.wallpaperId = wallpaperId;
        this.monitor = monitor;
        this.projectJsonModified = projectJsonModified;
    }

    public int getVersion() { return version; }
    public String getWallpaperId() { return wallpaperId; }
    public String getMonitor() { return monitor; }
    public long getProjectJsonModified() { return projectJsonModified; }
    public Map<String, String> getSettings() { return settings; }

    public String getGpuMode() { return gpuMode; }
    public void setGpuMode(String gpuMode) { this.gpuMode = gpuMode; }
    public int getDriPrimeNumber() { return driPrimeNumber; }
    public void setDriPrimeNumber(int n) { this.driPrimeNumber = n; }
    public boolean isGlThreadedOpt() { return glThreadedOpt; }
    public void setGlThreadedOpt(boolean v) { this.glThreadedOpt = v; }

    public void put(String key, String value) { settings.put(key, value); }
    public String get(String key) { return settings.get(key); }

    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        json.put("version", version);
        json.put("wallpaperId", wallpaperId);
        json.put("monitor", monitor);
        json.put("projectJsonModified", projectJsonModified);
        json.put("gpuMode", gpuMode);
        json.put("driPrimeNumber", driPrimeNumber);
        json.put("glThreadedOpt", glThreadedOpt);
        JSONObject s = new JSONObject();
        for (Map.Entry<String, String> e : settings.entrySet()) {
            s.put(e.getKey(), e.getValue());
        }
        json.put("settings", s);
        return json;
    }

    public static WallpaperConfig fromJson(JSONObject json) {
        WallpaperConfig cfg = new WallpaperConfig();
        cfg.version = json.optInt("version", CURRENT_VERSION);
        cfg.wallpaperId = json.optString("wallpaperId", "");
        cfg.monitor = json.optString("monitor", "");
        cfg.projectJsonModified = json.optLong("projectJsonModified", 0);
        cfg.gpuMode = json.optString("gpuMode", "DEFAULT");
        cfg.driPrimeNumber = json.optInt("driPrimeNumber", 1);
        cfg.glThreadedOpt = json.optBoolean("glThreadedOpt", false);
        if (json.has("settings")) {
            JSONObject s = json.getJSONObject("settings");
            for (String key : s.keySet()) {
                cfg.settings.put(key, s.getString(key));
            }
        }
        return cfg;
    }
}
