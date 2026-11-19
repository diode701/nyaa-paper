#pragma once

#include <string>
#include <vector>

namespace nyaa::io {

/// Where the app is allowed to read and write. Empty means "decide at runtime":
/// engine -> PATH lookup, assets -> linux-wallpaperengine's built-in assets.
struct PathConfig
{
    std::string workshop;
    std::string engine;
    std::string assets;
    std::string screenshots;
    std::string pywal;
};

struct PreviewConfig
{
    std::string animation = "always";   // always | hover | off
    int pageSlideMs = 170;
    int cachePages = 1;
};

/// Machine-level engine settings. Per-wallpaper choices (volume, fps, flags,
/// GPU, screenshot) are NOT here: like Java, they belong to each apply and are
/// saved per monitor in that monitor's WallpaperConfig.
struct EngineDefaults
{
    std::string layer = "bottom";       // Wayland: background | bottom | top | overlay
};

struct UiConfig
{
    std::string monitor;
    /// Output names Qt does not report, added by hand (Java: custom monitors
    /// in the launcher). Shown after the detected screens.
    std::vector<std::string> customMonitors;

    // ---- look (Settings → Appearance) ----
    std::string palette = "nyaa";          // a key of Theme's palettes
    std::string themeSource = "palette";   // palette | wallpaper | pywal | system
    std::string hoverStyle = "java";       // java (scale + border) | lift | off
    std::string selectionStyle = "brackets"; // brackets | outline | quiet
    bool backdrop = true;                  // blurred selected wallpaper behind the grid
    bool tint = false;                     // recolour the window from the selected wallpaper
    double scale = 1.0;                    // QT_SCALE_FACTOR, applied at start
    std::string renderer = "vulkan";       // vulkan | opengl | software, applied at start
    bool titleScreen = true;               // start on the title screen
};

/// Java Settings.persistOnStart / autoStartOnBoot
struct StartupConfig
{
    bool persistOnStart = true;     // restore each monitor's wallpaper at launch
    bool autoStartOnBoot = false;   // start hidden in the tray at login
};

/// The whole user configuration. A plain value: no singleton, no I/O.
struct AppConfig
{
    int version = 1;
    PathConfig paths;
    PreviewConfig preview;
    EngineDefaults engine;
    UiConfig ui;
    StartupConfig startup;
};

} // namespace nyaa::io
