package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.Spinner;
import javafx.scene.control.SpinnerValueFactory;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import java.util.List;
import java.util.function.Consumer;

public class FpsSetting extends GlobalOption {
    private int fps = 30; // LWE default
    private boolean enabled = false;

    public FpsSetting() {
        super("fps", "Frame Rate Limit (0 = unlimited)");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox container = new VBox(5);
        Label heading = new Label(description);
        heading.setStyle("-fx-font-weight: bold; -fx-text-fill: #e0e0e0;");

        HBox row = new HBox(10);
        CheckBox enableCheck = new CheckBox("Limit FPS");
        enableCheck.setSelected(enabled);

        Spinner<Integer> spinner = new Spinner<>(0, 240, fps);
        spinner.setEditable(true);
        spinner.setValueFactory(new SpinnerValueFactory.IntegerSpinnerValueFactory(0, 240, fps));
        spinner.disableProperty().bind(enableCheck.selectedProperty().not());

        enableCheck.setOnAction(e -> {
            enabled = enableCheck.isSelected();
            if (!enabled) fps = 0;
            onChange.accept(this);
        });

        spinner.valueProperty().addListener((obs, old, newVal) -> {
            fps = newVal;
            onChange.accept(this);
        });

        row.getChildren().addAll(enableCheck, spinner);
        container.getChildren().addAll(heading, row);
        return container;
    }

    @Override
    public List<String> toCommandArgs() {
        if (enabled && fps > 0) {
            return List.of("--fps", String.valueOf(fps));
        }
        return List.of();
    }

    @Override
    public String getValueString() { return enabled + "|" + fps; }

    @Override
    public void setValueFromString(String s) {
        String[] parts = s.split("\\|", 2);
        if (parts.length == 2) {
            enabled = "true".equals(parts[0]);
            try { fps = Integer.parseInt(parts[1]); } catch (NumberFormatException e) { fps = 30; }
        }
    }
}
