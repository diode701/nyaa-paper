package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import java.util.List;
import java.util.function.Consumer;

public class ScalingSetting extends GlobalOption {
    private String scaling = "default";
    private static final List<String> OPTIONS = List.of("stretch", "fit", "fill", "default");

    public ScalingSetting() {
        super("scaling", "Wallpaper Scaling");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        Label heading = new Label(description);
        heading.setStyle("-fx-font-weight: bold; -fx-text-fill: #e0e0e0;");
        box.getChildren().add(heading);
        ComboBox<String> combo = new ComboBox<>();
        combo.getItems().addAll(OPTIONS);
        combo.setValue(scaling);
        combo.setOnAction(e -> {
            scaling = combo.getValue();
            onChange.accept(this);
        });
        box.getChildren().add(combo);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        if (!scaling.equals("default")) {
            return List.of("--scaling", scaling);
        }
        return List.of();
    }

    @Override
    public String getValueString() { return scaling; }

    @Override
    public void setValueFromString(String s) {
        if (s != null && OPTIONS.contains(s)) scaling = s;
    }

    @Override
    public boolean isPrePropertyFlag() {
        return false;
    }

}