package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import javafx.scene.text.TextAlignment;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;

public class TextProperty extends Setting {

    public TextProperty(String name, String description) {
        super(name, description);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        Label label = new Label(description);
        label.setWrapText(true);
        label.setStyle("-fx-text-fill: #aaaaaa; -fx-font-size: 13;");
        label.setTextAlignment(TextAlignment.LEFT);
        box.getChildren().add(label);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        return Collections.emptyList();
    }
}
