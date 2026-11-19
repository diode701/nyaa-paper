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

struct EngineDefaults
{
    int volume = 70;
    bool muted = false;
    int fps = 30;
    bool fpsLimited = false;
    std::string scaling = "default";
    std::string clamp;
    std::string layer = "bottom";       // Wayland: background | bottom | top | overlay
    std::vector<std::string> flags;
};

struct UiConfig
{
    std::string monitor;
};

/// The whole user configuration. A plain value: no singleton, no I/O.
struct AppConfig
{
    int version = 1;
    PathConfig paths;
    PreviewConfig preview;
    EngineDefaults engine;
    UiConfig ui;
};

} // namespace nyaa::io
