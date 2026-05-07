package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.layout.VBox;
import javafx.scene.text.Font;
import javafx.scene.text.FontWeight;
import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;

public class GroupProperty extends Setting {

    public GroupProperty(String name, String description) {
        super(name, description);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        box.setStyle("-fx-background-color: #333333; -fx-background-radius: 3; -fx-padding: 8;");
        Label label = new Label(description);
        label.setWrapText(true);
        label.setFont(Font.font(null, FontWeight.BOLD, 14));
        label.setStyle("-fx-text-fill: #e0e0e0;");
        box.getChildren().add(label);
        return box;
    }

    @Override
    public List<String> toCommandArgs() {
        return Collections.emptyList();
    }
}
