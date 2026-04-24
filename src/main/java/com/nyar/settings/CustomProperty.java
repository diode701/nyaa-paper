package com.nyar.settings;

import java.util.List;

public abstract class CustomProperty<T> extends Setting {
    protected T value;

    public CustomProperty(String name, String description, T value) {
        super(name, description);
        this.value = value;
    }

    public T getValue() { return value; }
    public void setValue(T value) { this.value = value; }

    @Override
    public List<String> toCommandArgs() {
        return List.of("--set-property", name + "=" + valueToString());
    }

    protected abstract String valueToString();
}