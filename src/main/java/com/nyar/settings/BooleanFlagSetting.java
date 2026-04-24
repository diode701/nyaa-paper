package com.nyar.settings;

import javafx.scene.Node;
import javafx.scene.control.CheckBox;
import java.util.List;
import java.util.function.Consumer;

public class BooleanFlagSetting extends GlobalOption {
    private final String flagName;
    private boolean enabled = false;

    public BooleanFlagSetting(String flagName, String description) {
        super(flagName, description);
        this.flagName = flagName;
    }

    @Override
    public Node createControl(Consumer<Setting> onChange) {
        CheckBox checkBox = new CheckBox(description);
        checkBox.setSelected(enabled);
        checkBox.setOnAction(e -> {
            enabled = checkBox.isSelected();
            onChange.accept(this);
        });
        return checkBox;
    }

    @Override
    public List<String> toCommandArgs() {
        if (enabled) {
            return List.of("--" + flagName);
        }
        return List.of();
    }
}