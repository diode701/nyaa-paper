package com.nyar.settings;

import javafx.scene.Node;
import java.util.List;
import java.util.function.Consumer;

public abstract class Setting {
    protected final String name;
    protected final String description;

    public Setting(String name, String description) {
        this.name = name;
        this.description = description;
    }

    public String getName() { return name; }
    public String getDescription() { return description; }

    public abstract Node createControl(Consumer<Setting> onChange);
    public abstract List<String> toCommandArgs();
}