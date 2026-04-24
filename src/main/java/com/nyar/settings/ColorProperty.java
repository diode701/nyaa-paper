package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.ColorPicker;
import javafx.scene.paint.Color;
import java.util.function.Consumer;

public class ColorProperty extends CustomProperty<Color> {
    public ColorProperty(String name, String description, Color value) {
        super(name, description, value);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        ColorPicker picker = new ColorPicker(value);
        picker.setOnAction(e -> {
            this.value = picker.getValue();
            onChange.accept(this);
        });
        return picker;
    }

    @Override
    protected String valueToString() {
        return String.format("#%02X%02X%02X",
                (int)(value.getRed() * 255),
                (int)(value.getGreen() * 255),
                (int)(value.getBlue() * 255));
    }
}