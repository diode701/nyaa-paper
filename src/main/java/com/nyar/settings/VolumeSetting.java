package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.control.Slider;
import javafx.scene.layout.HBox;
import java.util.List;
import java.util.function.Consumer;

public class VolumeSetting extends GlobalOption {
    private int volume = 15; // LWE default is 15
    private static final int MIN = 0;
    private static final int MAX = 100;
    private static final int STEP = 5;

    public VolumeSetting() {
        super("volume", "Audio Volume (0-100)");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        HBox box = new HBox(10);
        Slider slider = new Slider(MIN, MAX, volume);
        slider.setShowTickMarks(true);
        slider.setMajorTickUnit(25);
        slider.setBlockIncrement(STEP);
        Label valueLabel = new Label(String.valueOf(volume));

        slider.valueProperty().addListener((obs, old, newVal) -> {
            volume = newVal.intValue();
            valueLabel.setText(String.valueOf(volume));
            onChange.accept(this);
        });

        box.getChildren().addAll(slider, valueLabel);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        // Volume expects integer 0-100, not float
        return List.of("--volume", String.valueOf(volume));
    }
}
