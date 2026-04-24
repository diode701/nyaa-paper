package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import java.util.Map;
import java.util.function.Consumer;

public class ComboProperty extends CustomProperty<Integer> {
    private final Map<Integer, String> options;

    public ComboProperty(String name, String description, int value, Map<Integer, String> options) {
        super(name, description, value);
        this.options = options;
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        box.getChildren().add(new Label(description));
        ComboBox<String> combo = new ComboBox<>();
        combo.getItems().addAll(options.values());
        String currentText = options.get(value);
        if (currentText != null) combo.setValue(currentText);
        combo.setOnAction(e -> {
            for (Map.Entry<Integer, String> entry : options.entrySet()) {
                if (entry.getValue().equals(combo.getValue())) {
                    this.value = entry.getKey();
                    onChange.accept(this);
                    break;
                }
            }
        });
        box.getChildren().add(combo);
        return box;
    }

    @Override
    protected String valueToString() {
        return String.valueOf(value);
    }
}