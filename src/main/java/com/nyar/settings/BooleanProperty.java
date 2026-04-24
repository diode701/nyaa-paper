package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import java.util.function.Consumer;

public class BooleanProperty extends CustomProperty<Boolean> {
    public BooleanProperty(String name, String description, boolean value) {
        super(name, description, value);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        CheckBox checkBox = new CheckBox(description);
        checkBox.setSelected(value);
        checkBox.setOnAction(e -> {
            this.value = checkBox.isSelected();
            onChange.accept(this);
        });
        return checkBox;
    }

    @Override
    protected String valueToString() {
        return value ? "1" : "0";
    }
}