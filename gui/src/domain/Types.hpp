#pragma once

#include <string>
#include <vector>

namespace nyaa::domain {

/// A library entry, as facts only. The daemon reports these; nothing here is
/// derived from project.json's property block.
struct Wallpaper
{
    std::string id;
    std::string title;
    std::string type;
    std::string file;
    std::string basePath;
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
