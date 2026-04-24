package com.nyar;

import org.json.JSONObject;
import java.util.*;

public class Wppr {
    private final String id;
    private final String title;
    private final String type;
    private final String basePath;
    private final JSONObject properties;  // Store raw properties from JSON
    private final Map<String, Object> parsedProperties;  // Cache parsed values

    public Wppr(String id, String title, String type, String basePath, JSONObject properties) {
        this.id = id;
        this.title = title;
        this.type = type;
        this.basePath = basePath;
        this.properties = properties != null ? properties : new JSONObject();
        this.parsedProperties = new HashMap<>();
    }

    public String getId() { return id; }
    public String getTitle() { return title; }
    public String getType() { return type; }
    public String getBasePath() { return basePath; }

    public String getPreviewPath() {
        return basePath + "/preview.jpg";  // or .gif
    }

    // Check if wallpaper has customizable properties
    public boolean hasProperties() {
        return properties.length() > 0;
    }

    // Get raw properties JSON (for parsing in property view)
    public JSONObject getPropertiesJSON() {
        return properties;
    }

    // Get a specific property value (cached)
    public Object getProperty(String key) {
        if (!parsedProperties.containsKey(key) && properties.has(key)) {
            parsedProperties.put(key, properties.get(key));
        }
        return parsedProperties.get(key);
    }

    // Get all property keys
    public Set<String> getPropertyKeys() {
        return properties.keySet();
    }
}
