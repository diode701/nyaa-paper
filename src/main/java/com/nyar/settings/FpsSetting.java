package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Spinner;
import javafx.scene.control.SpinnerValueFactory;
import javafx.scene.layout.HBox;
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
        HBox box = new HBox(10);
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

        box.getChildren().addAll(enableCheck, spinner);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        if (enabled && fps > 0) {
            return List.of("--fps", String.valueOf(fps)); // Integer
        }
        return List.of();
    }
}
