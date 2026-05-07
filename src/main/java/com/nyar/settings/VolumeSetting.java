package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import javafx.scene.control.Label;
import javafx.scene.control.Slider;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import java.util.List;
import java.util.function.Consumer;

public class VolumeSetting extends GlobalOption {
    private int volume = 70;
    private boolean muted = false;
    private static final int MIN = 0;
    private static final int MAX = 100;
    private static final int STEP = 5;

    public VolumeSetting() {
        super("volume", "Audio");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox container = new VBox(5);
        Label heading = new Label(description);
        heading.setStyle("-fx-font-weight: bold; -fx-text-fill: #e0e0e0;");

        HBox row = new HBox(10);
        Slider slider = new Slider(MIN, MAX, volume);
        slider.setShowTickMarks(true);
        slider.setMajorTickUnit(25);
        slider.setBlockIncrement(STEP);
        Label valueLabel = new Label(String.valueOf(volume));

        CheckBox muteCheck = new CheckBox("Mute");
        muteCheck.setSelected(muted);
        slider.disableProperty().bind(muteCheck.selectedProperty());
        valueLabel.disableProperty().bind(muteCheck.selectedProperty());

        muteCheck.setOnAction(e -> {
            muted = muteCheck.isSelected();
            onChange.accept(this);
        });

        slider.valueProperty().addListener((obs, old, newVal) -> {
            volume = newVal.intValue();
            valueLabel.setText(String.valueOf(volume));
            onChange.accept(this);
        });

        row.getChildren().addAll(muteCheck, slider, valueLabel);
        container.getChildren().addAll(heading, row);
        return container;
    }

    @Override
    public List<String> toCommandArgs() {
        if (muted) {
            return List.of("--silent");
        }
        return List.of("--volume", String.valueOf(volume));
    }

    @Override
    public String getValueString() {
        return muted ? "silent|" + volume : String.valueOf(volume);
    }

    @Override
    public void setValueFromString(String s) {
        if (s.startsWith("silent|")) {
            muted = true;
            try { volume = Integer.parseInt(s.substring(7)); } catch (NumberFormatException e) { volume = 70; }
        } else {
            muted = false;
            try { volume = Integer.parseInt(s); } catch (NumberFormatException e) { volume = 70; }
        }
    }
}
