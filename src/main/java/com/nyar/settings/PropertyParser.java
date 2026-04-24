package com.nyar.settings;

import com.nyar.Wppr;
import javafx.scene.paint.Color;
import org.json.JSONObject;
import java.util.*;

public class PropertyParser {

    public static List<CustomProperty<?>> parseFromWppr(Wppr wallpaper) {
        List<CustomProperty<?>> properties = new ArrayList<>();

        if (!wallpaper.hasProperties()) {
            return properties;
        }

        JSONObject props = wallpaper.getPropertiesJSON();
        for (String key : props.keySet()) {
            JSONObject prop = props.getJSONObject(key);
            String type = prop.getString("type");
            String text = prop.optString("text", key);

            CustomProperty<?> setting = createProperty(key, text, type, prop);
            if (setting != null) {
                properties.add(setting);
            }
        }

        return properties;
    }

    private static CustomProperty<?> createProperty(String name, String text, String type, JSONObject prop) {
        switch (type) {
            case "bool":
                boolean boolVal = prop.optBoolean("value", false);
                return new BooleanProperty(name, text, boolVal);

            case "slider":
                double val = prop.optDouble("value", 0);
                double min = prop.optDouble("min", 0);
                double max = prop.optDouble("max", 100);
                double step = prop.optDouble("step", 1);
                return new SliderProperty(name, text, val, min, max, step);

            case "color":
                String colorStr = prop.optString("value", "0 0 0");
                Color color = parseColor(colorStr);
                return new ColorProperty(name, text, color);

            case "combo":
                // Handle combo boxes if present
                int intVal = prop.optInt("value", 0);
                Map<Integer, String> options = new LinkedHashMap<>();
                if (prop.has("options")) {
                    JSONObject opts = prop.getJSONObject("options");
                    for (String optKey : opts.keySet()) {
                        options.put(Integer.parseInt(optKey), opts.getString(optKey));
                    }
                }
                return new ComboProperty(name, text, intVal, options);

            default:
                System.out.println("Unknown property type: " + type);
                return null;
        }
    }

    private static Color parseColor(String colorStr) {
        String[] parts = colorStr.trim().split("\\s+");
        if (parts.length >= 3) {
            try {
                double r = Double.parseDouble(parts[0]);
                double g = Double.parseDouble(parts[1]);
                double b = Double.parseDouble(parts[2]);
                return Color.color(r, g, b);
            } catch (NumberFormatException e) {
                return Color.BLACK;
            }
        }
        return Color.BLACK;
    }
}
