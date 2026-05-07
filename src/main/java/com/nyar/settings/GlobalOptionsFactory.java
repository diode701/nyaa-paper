package com.nyar.settings;

import java.util.List;

public class GlobalOptionsFactory {
    private GlobalOptionsFactory() {}

    public static List<GlobalOption> createAll() {
        return List.of(
                new VolumeSetting(),
                new FpsSetting(),
                new ScalingSetting(),
                new ClampSetting(),
                new BooleanFlagSetting("disable-mouse", "Disable Mouse Interaction"),
                new BooleanFlagSetting("disable-parallax", "Disable Parallax"),
                new BooleanFlagSetting("noautomute", "Don't Auto-mute"),
                new BooleanFlagSetting("no-audio-processing", "Disable Audio Processing"),
                new BooleanFlagSetting("no-fullscreen-pause", "Don't Pause on Fullscreen"),
                new BooleanFlagSetting("fullscreen-pause-only-active",
                        "Pause Only When Active Fullscreen (Wayland)"),
                new BooleanFlagSetting("disable-particles", "Disable particles")
        );
    }
}
