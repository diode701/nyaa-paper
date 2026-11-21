#pragma once

#include <string>
#include <vector>

namespace nyaa::domain {

/// One subscribed wallpaper. Java: Wppr. The daemon reports these facts;
/// the property block of project.json is read separately when it is opened.
struct Wallpaper
{
    std::string id;
    std::string title;
    std::string type;       // "scene", "video", "web", ...
    std::string file;       // project.json "file" (the video for video wallpapers)
    std::string basePath;   // the wallpaper's folder
    std::string preview;    // preview.jpg / .gif / .png, empty if none
    /// Workshop item this one is a preset of; empty unless this is a preset
    std::string dependency;
    /// Preset values, already formatted for --set-property (key, value)
    std::vector<std::pair<std::string, std::string>> preset;

    bool isPreset() const { return !dependency.empty(); }
    bool isVideo() const { return type == "video" || type == "Video"; }
};

struct GpuConfig
{
    enum class Mode { Default, DriPrime, Nvidia };

    Mode mode = Mode::Default;
    int driPrimeNumber = 1;
    bool glThreaded = false;
};

struct Screenshot
{
    std::string directory;   // empty = disabled
    int delay = 0;

    bool enabled() const { return !directory.empty(); }
};

/// Engine invocation details that are configuration, not wallpaper data.
struct EngineOptions
{
    std::string workshopRoot;   // used to decide id vs path for --bg
    std::string assetsDir;      // empty: let the engine use its built-in assets
    std::string layer;          // Wayland layer; empty: don't pass the flag
};

/// Exactly what the daemon will execute: the binary is implied, everything the
/// caller wants appended is here, and the environment is "KEY=VALUE" strings.
struct EngineCommand
{
    std::string wallpaperId;
    std::vector<std::string> argv;
    std::vector<std::string> env;

    bool empty() const { return argv.empty(); }
};

} // namespace nyaa::domain
