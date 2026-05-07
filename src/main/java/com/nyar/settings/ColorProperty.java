package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.ColorPicker;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import javafx.scene.paint.Color;
import java.util.function.Consumer;

public class ColorProperty extends CustomProperty<Color> {
    public ColorProperty(String name, String description, Color value) {
        super(name, description, value);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        ColorPicker picker = new ColorPicker(value);
        picker.setOnAction(e -> {
            this.value = picker.getValue();
            onChange.accept(this);
        });
        box.getChildren().addAll(new Label(description), picker);
        return box;
    }

    @Override
    protected String valueToString() {
        return String.format("%.4f %.4f %.4f",
                value.getRed(), value.getGreen(), value.getBlue());
    }

    @Override
    public String getValueString() { return valueToString(); }

    @Override
    public void setValueFromString(String s) {
        try {
            String[] parts = s.trim().split("\\s+");
            if (parts.length >= 3) {
                double r = Double.parseDouble(parts[0]);
                double g = Double.parseDouble(parts[1]);
                double b = Double.parseDouble(parts[2]);
                this.value = Color.color(r, g, b);
            }
        } catch (NumberFormatException ignored) {}
    }
}