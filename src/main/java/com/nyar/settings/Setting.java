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

    public String getValueString() { return null; }
    public void setValueFromString(String s) { /* no-op – override in subclasses that carry state */ }
    /**
     * Controls where this setting’s arguments appear in the command line.
     * @return true if arguments go <b>before</b> --set-property (and other per‑property flags),
     *         false if they go <b>after</b> (like scaling, clamp, custom properties).
     */
    public boolean isPrePropertyFlag() {
        return true;
    }

}