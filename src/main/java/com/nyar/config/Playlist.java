package com.nyar.config;

import org.json.JSONArray;
import org.json.JSONObject;
import java.util.ArrayList;
import java.util.List;

public class Playlist {

    public enum Mode { SEQUENTIAL, RANDOM }

    private Mode mode = Mode.SEQUENTIAL;
    private int intervalSec = 60;
    private int currentIndex = 0;
    private boolean loop = true;
    private final List<WallpaperConfig> entries = new ArrayList<>();

    public Playlist() {}

    public Mode getMode() { return mode; }
    public void setMode(Mode mode) { this.mode = mode; }

    public int getIntervalSec() { return intervalSec; }
    public void setIntervalSec(int intervalSec) { this.intervalSec = intervalSec; }

    public int getCurrentIndex() { return currentIndex; }
    public void setCurrentIndex(int currentIndex) { this.currentIndex = currentIndex; }

    public boolean isLoop() { return loop; }
    public void setLoop(boolean loop) { this.loop = loop; }

    public List<WallpaperConfig> getEntries() { return entries; }

    public JSONObject toJson() {
        JSONObject json = new JSONObject();
        json.put("mode", mode.name());
        json.put("intervalSec", intervalSec);
        json.put("currentIndex", currentIndex);
        json.put("loop", loop);
        JSONArray arr = new JSONArray();
        for (WallpaperConfig entry : entries) {
            arr.put(entry.toJson());
        }
        json.put("entries", arr);
        return json;
    }

    public static Playlist fromJson(JSONObject json) {
        Playlist p = new Playlist();
        String modeStr = json.optString("mode", "SEQUENTIAL");
        try {
            p.mode = Mode.valueOf(modeStr);
        } catch (IllegalArgumentException e) {
            p.mode = Mode.SEQUENTIAL;
        }
        p.intervalSec = json.optInt("intervalSec", 60);
        p.currentIndex = json.optInt("currentIndex", 0);
        p.loop = json.optBoolean("loop", true);
        if (json.has("entries")) {
            JSONArray arr = json.getJSONArray("entries");
            for (int i = 0; i < arr.length(); i++) {
                WallpaperConfig cfg = WallpaperConfig.fromJson(arr.getJSONObject(i));
                p.entries.add(cfg);
            }
        }
        return p;
    }
}
