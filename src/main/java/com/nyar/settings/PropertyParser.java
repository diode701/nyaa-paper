package com.nyar.settings;

import com.nyar.Wppr;
import javafx.scene.paint.Color;
import org.json.JSONArray;
import org.json.JSONObject;
import java.util.*;

public class PropertyParser {

    public static List<Setting> parseFromWppr(Wppr wallpaper) {
        if (!wallpaper.hasProperties()) {
            return Collections.emptyList();
        }

        JSONObject props = wallpaper.getPropertiesJSON();

        // Sort by order field so properties appear in the sequence intended by the author
        List<Map.Entry<String, Integer>> sortedKeys = new ArrayList<>();
        for (String key : props.keySet()) {
            JSONObject prop = props.getJSONObject(key);
            int order = prop.optInt("order", Integer.MAX_VALUE);
            sortedKeys.add(new AbstractMap.SimpleEntry<>(key, order));
        }
        sortedKeys.sort(Comparator.comparingInt(Map.Entry::getValue));

        List<Setting> properties = new ArrayList<>();
        for (Map.Entry<String, Integer> entry : sortedKeys) {
            String key = entry.getKey();
            try {
                JSONObject prop = props.getJSONObject(key);
                Setting setting = parseProperty(key, prop);
                if (setting != null) {
                    properties.add(setting);
                }
            } catch (Exception e) {
                System.out.println("Skipping malformed property '" + key + "': " + e.getMessage());
            }
        }

        return properties;
    }

    private static Setting parseProperty(String key, JSONObject prop) {
        // Default to "text" if type is missing (some wallpapers omit the type field for labels)
        String type = prop.optString("type", "text");
        String text = cleanHtml(prop.optString("text", key));

        switch (type) {
            case "bool":
                return parseBoolean(key, text, prop);
            case "slider":
                return parseSlider(key, text, prop);
            case "color":
                return parseColor_(key, text, prop);
            case "combo":
                return parseCombo(key, text, prop);
            case "text":
                return new TextProperty(key, text);
            case "group":
                return new GroupProperty(key, text);
            case "textinput":
                return parseTextInput(key, text, prop);
            case "file":
                return parseFile(key, text, prop);
            case "directory":
                return parseDirectory(key, text, prop);
            case "scenetexture":
                return parseTexture(key, text, prop);
            default:
                System.out.println("Unknown property type: " + type + " (key: " + key + ")");
                return null;
        }
    }

    // ── bool ──
    private static BooleanProperty parseBoolean(String name, String text, JSONObject prop) {
        boolean boolVal = false;
        if (prop.has("value")) {
            Object raw = prop.get("value");
            if (raw instanceof Boolean) {
                boolVal = (Boolean) raw;
            } else if (raw instanceof Number) {
                boolVal = ((Number) raw).intValue() != 0;
            } else {
                String s = raw.toString().trim().toLowerCase();
                boolVal = s.equals("true") || s.equals("1");
            }
        }
        return new BooleanProperty(name, text, boolVal);
    }

    // ── slider ──
    private static SliderProperty parseSlider(String name, String text, JSONObject prop) {
        double val = parseDouble(prop, "value", 0);
        double min = parseDouble(prop, "min", 0);
        double max = parseDouble(prop, "max", 100);
        double step = parseDouble(prop, "step", 1);
        if (step <= 0) step = 1;
        return new SliderProperty(name, text, val, min, max, step);
    }

    // ── color ──
    private static ColorProperty parseColor_(String name, String text, JSONObject prop) {
        String colorStr = prop.optString("value", "0 0 0");
        return new ColorProperty(name, text, parseColorValue(colorStr));
    }

    private static Color parseColorValue(String raw) {
        String trimmed = raw.trim();
        // Hex format: "#RRGGBB" or "RRGGBB"
        if (trimmed.startsWith("#") || trimmed.matches("^[0-9A-Fa-f]{6}$")) {
            try {
                return Color.web(trimmed.startsWith("#") ? trimmed : "#" + trimmed);
            } catch (Exception e) {
                return Color.BLACK;
            }
        }
        // Space-separated float triples (normalised 0–1 or 0–255)
        String[] parts = trimmed.split("\\s+");
        if (parts.length >= 3) {
            try {
                double r = Double.parseDouble(parts[0]);
                double g = Double.parseDouble(parts[1]);
                double b = Double.parseDouble(parts[2]);
                // Heuristic: if any channel > 1, assume 0–255 range
                if (r > 1 || g > 1 || b > 1) {
                    r /= 255.0;
                    g /= 255.0;
                    b /= 255.0;
                }
                return Color.color(r, g, b);
            } catch (NumberFormatException e) {
                return Color.BLACK;
            }
        }
        return Color.BLACK;
    }

    // ── combo ──
    private static ComboProperty parseCombo(String name, String text, JSONObject prop) {
        // Parse current value
        int intVal = 0;
        if (prop.has("value")) {
            Object rawVal = prop.get("value");
            if (rawVal instanceof Number) {
                intVal = ((Number) rawVal).intValue();
            } else {
                try {
                    intVal = Integer.parseInt(rawVal.toString());
                } catch (NumberFormatException e) {
                    intVal = 0;
                }
            }
        }

        // Parse options
        Map<Integer, String> options = new LinkedHashMap<>();
        if (prop.has("options")) {
            JSONArray arr = prop.getJSONArray("options");
            for (int i = 0; i < arr.length(); i++) {
                JSONObject item = arr.getJSONObject(i);
                String label = item.optString("label", item.optString("text", "?"));
                int valueKey;
                Object rawOptVal = item.get("value");
                if (rawOptVal instanceof Number) {
                    valueKey = ((Number) rawOptVal).intValue();
                } else {
                    valueKey = Integer.parseInt(rawOptVal.toString());
                }
                options.put(valueKey, label);
            }
        }
        return new ComboProperty(name, text, intVal, options);
    }

    // ── textinput ──
    private static TextInputProperty parseTextInput(String name, String text, JSONObject prop) {
        String val = "";
        if (prop.has("value")) {
            val = prop.get("value").toString();
        }
        return new TextInputProperty(name, text, val);
    }

    // ── file ──
    private static FileProperty parseFile(String name, String text, JSONObject prop) {
        String val = prop.optString("value", "");
        return new FileProperty(name, text, val);
    }

    // ── directory ──
    private static DirectoryProperty parseDirectory(String name, String text, JSONObject prop) {
        String val = prop.optString("value", "");
        return new DirectoryProperty(name, text, val);
    }

    // ── scenetexture ──
    private static TextureProperty parseTexture(String name, String text, JSONObject prop) {
        String val = prop.optString("value", "");
        return new TextureProperty(name, text, val);
    }

    // ── HTML cleaner ──
    // Strips unsupported HTML tags from property labels so they render cleanly in JavaFX Labels.
    // Converts <br> variants to newlines and strips all other tags, keeping inner text.
    static String cleanHtml(String html) {
        if (html == null || html.isEmpty()) return "";
        String s = html;
        // <br> variants → newline
        s = s.replaceAll("(?i)<br\\s*/?>", "\n");
        // Remove all other HTML tags (preserve inner text)
        s = s.replaceAll("<[^>]+>", "");
        // Decode common entities
        s = s.replace("&amp;", "&");
        s = s.replace("&lt;", "<");
        s = s.replace("&gt;", ">");
        s = s.replace("&quot;", "\"");
        s = s.replace("&apos;", "'");
        s = s.replace("&nbsp;", " ");
        // Collapse runs of 3+ newlines down to 2
        s = s.replaceAll("\n{3,}", "\n\n");
        return s.trim();
    }

    // ── helpers ──
    private static double parseDouble(JSONObject obj, String key, double fallback) {
        if (!obj.has(key)) return fallback;
        Object raw = obj.get(key);
        if (raw instanceof Number) return ((Number) raw).doubleValue();
        try {
            return Double.parseDouble(raw.toString());
        } catch (NumberFormatException e) {
            return fallback;
        }
    }


}
