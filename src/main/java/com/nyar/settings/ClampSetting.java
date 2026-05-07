package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import java.util.List;
import java.util.function.Consumer;

public class ClampSetting extends GlobalOption {
    private boolean enabled = false;
    private String mode = "border";                 // default mode when enabled
    private static final List<String> OPTIONS = List.of("clamp", "border", "repeat");

    public ClampSetting() {
        super("clamp", "Texture Clamping");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox container = new VBox(5);
        Label heading = new Label(description);
        heading.setStyle("-fx-font-weight: bold; -fx-text-fill: #e0e0e0;");
        container.getChildren().add(heading);

        HBox row = new HBox(10);

        CheckBox enableCheck = new CheckBox("Enable clamping");
        enableCheck.setSelected(enabled);

        ComboBox<String> combo = new ComboBox<>();
        combo.getItems().addAll(OPTIONS);
        combo.setValue(mode);
        combo.disableProperty().bind(enableCheck.selectedProperty().not());

        enableCheck.setOnAction(e -> {
            enabled = enableCheck.isSelected();
            onChange.accept(this);
        });

        combo.setOnAction(e -> {
            mode = combo.getValue();
            onChange.accept(this);
        });

        row.getChildren().addAll(enableCheck, combo);
        container.getChildren().add(row);
        return container;
    }

    @Override
    public List<String> toCommandArgs() {
        if (enabled && mode != null && !mode.isEmpty()) {
            return List.of("--clamp", mode);
        }
        return List.of();
    }

    @Override
    public String getValueString() { return enabled + "|" + mode; }

    @Override
    public void setValueFromString(String s) {
        String[] parts = s.split("\\|", 2);
        if (parts.length == 2) {
            enabled = "true".equals(parts[0]);
            if (OPTIONS.contains(parts[1])) mode = parts[1];
        }
    }

    @Override
    public boolean isPrePropertyFlag() {
        return false;
    }
}
