package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.ComboBox;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import java.util.List;
import java.util.function.Consumer;
//deprecated
public class ClampingSetting extends GlobalOption {
    private String clamping = "";
    private static final List<String> OPTIONS = List.of("clamp", "border", "repeat");

    public ClampingSetting() {
        super("clamping", "Texture Clamping");
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        box.getChildren().add(new Label(description));
        ComboBox<String> combo = new ComboBox<>();
        combo.getItems().addAll(OPTIONS);
        combo.setValue(clamping);
        combo.setOnAction(e -> {
            clamping = combo.getValue();
            onChange.accept(this);
        });
        box.getChildren().add(combo);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        return List.of("--clamping", clamping);
    }
}