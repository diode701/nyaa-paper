package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.control.Slider;
import javafx.scene.layout.HBox;
import java.util.function.Consumer;

public class SliderProperty extends CustomProperty<Double> {
    private final double min;
    private final double max;
    private final double step;

    public SliderProperty(String name, String description, double value,
                          double min, double max, double step) {
        super(name, description, value);
        this.min = min;
        this.max = max;
        this.step = step;
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        HBox box = new HBox(10);
        Slider slider = new Slider(min, max, value);
        slider.setBlockIncrement(step);
        if ((max - min) / Math.max(step, 0.001) <= 200) {
            slider.setShowTickMarks(true);
            slider.setMajorTickUnit(Math.max(step, (max - min) / 10));
        }
        Label valueLabel = new Label(String.format("%.2f", value));
        slider.valueProperty().addListener((obs, old, newVal) -> {
            this.value = newVal.doubleValue();
            valueLabel.setText(String.format("%.2f", newVal.doubleValue()));
            onChange.accept(this);
        });
        box.getChildren().addAll(new Label(description), slider, valueLabel);
        return box;
    }

    @Override
    protected String valueToString() {
        return String.format("%.2f", value);
    }

    @Override
    public String getValueString() { return valueToString(); }

    @Override
    public void setValueFromString(String s) {
        try { value = Double.parseDouble(s); } catch (NumberFormatException ignored) {}
    }
}