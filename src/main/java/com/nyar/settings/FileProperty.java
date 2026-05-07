package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.HBox;
import javafx.scene.layout.VBox;
import javafx.stage.FileChooser;
import java.io.File;
import java.util.function.Consumer;

public class FileProperty extends CustomProperty<String> {

    public FileProperty(String name, String description, String value) {
        super(name, description, value);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        box.getChildren().add(new Label(description));

        HBox row = new HBox(8);
        TextField field = new TextField(value);
        field.setMaxWidth(350);
        field.textProperty().addListener((obs, old, newVal) -> {
            this.value = newVal;
            onChange.accept(this);
        });

        Button browseBtn = new Button("Browse...");
        browseBtn.setStyle("-fx-background-color: #444; -fx-text-fill: white;");
        browseBtn.setOnAction(e -> {
            FileChooser chooser = new FileChooser();
            chooser.setTitle("Select file for " + description);
            File selected = chooser.showOpenDialog(browseBtn.getScene().getWindow());
            if (selected != null) {
                field.setText(selected.getAbsolutePath());
            }
        });

        row.getChildren().addAll(field, browseBtn);
        box.getChildren().add(row);
        return box;
    }

    @Override
    protected String valueToString() { return value; }

    @Override
    public String getValueString() { return value; }

    @Override
    public void setValueFromString(String s) { this.value = s; }
}
