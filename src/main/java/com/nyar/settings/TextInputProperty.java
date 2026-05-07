package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.VBox;
import java.util.function.Consumer;

public class TextInputProperty extends CustomProperty<String> {

    public TextInputProperty(String name, String description, String value) {
        super(name, description, value);
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        VBox box = new VBox(5);
        TextField field = new TextField(value);
        field.setMaxWidth(400);
        field.textProperty().addListener((obs, old, newVal) -> {
            this.value = newVal;
            onChange.accept(this);
        });
        box.getChildren().addAll(new Label(description), field);
        return box;
    }

    @Override
    protected String valueToString() { return value; }

    @Override
    public String getValueString() { return value; }

    @Override
    public void setValueFromString(String s) { this.value = s; }
}
