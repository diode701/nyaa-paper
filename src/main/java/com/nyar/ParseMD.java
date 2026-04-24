package com.nyar;

import org.json.JSONObject;

public class ParseMD {
    public static Wppr fromJson(String jContent, String id, String basePath) {
        try {
            JSONObject json = new JSONObject(jContent);

            String title = json.optString("title", "Unknown Title");
            String type = json.optString("type", "unknown");

            // Extract properties from the "general" object
            JSONObject properties = null;
            if (json.has("general")) {
                JSONObject general = json.getJSONObject("general");
                if (general.has("properties")) {
                    properties = general.getJSONObject("properties");
                }
            }

            return new Wppr(id, title, type, basePath, properties);

        } catch (Exception e) {
            System.out.println("Failed to parse JSON for " + id + ": " + e.getMessage());
            return null;
        }
    }
}
