#pragma once

#include <map>
#include <string>

#include "domain/Types.hpp"

namespace nyaa::domain {

/// What was applied on one monitor: the wallpaper, every setting value (its
/// own properties and the global options, by name) and the GPU it ran on.
///
/// Java equivalent: com.nyar.config.WallpaperConfig. Each monitor keeps its
/// own record, so each monitor can run a different wallpaper on a different
/// GPU with different options. The same record is what a playlist entry holds.
struct WallpaperConfig
{
    static constexpr int kCurrentVersion = 1;

    int version = kCurrentVersion;
    std::string wallpaperId;
    std::string monitor;
    long long projectJsonModified = 0;          // ms since epoch, like Java
    std::map<std::string, std::string> settings; // name -> getValueString()
    GpuConfig gpu;

    bool empty() const { return wallpaperId.empty(); }
};

/// Java's GpuConfig.Mode names, used in the saved files.
inline std::string gpuModeName(GpuConfig::Mode mode)
{
    switch (mode) {
    case GpuConfig::Mode::DriPrime: return "DRI_PRIME";
    case GpuConfig::Mode::Nvidia:   return "NVIDIA";
    case GpuConfig::Mode::Default:  break;
    }
    return "DEFAULT";
}

/// Unknown names fall back to DEFAULT, like Java's valueOf in a try/catch.
inline GpuConfig::Mode gpuModeFromName(const std::string &name)
{
    if (name == "DRI_PRIME") return GpuConfig::Mode::DriPrime;
    if (name == "NVIDIA")    return GpuConfig::Mode::Nvidia;
    return GpuConfig::Mode::Default;
}

} // namespace nyaa::domain
