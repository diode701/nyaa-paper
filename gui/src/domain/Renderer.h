#pragma once

#include <string>

#include "domain/Types.hpp"
#include "domain/options/GlobalOptions.h"
#include "domain/settings/SettingList.h"

namespace nyaa::domain {

/// Turns a wallpaper plus its settings into the exact command the daemon runs.
///
/// This is the only class that knows linux-wallpaperengine's CLI. Java
/// equivalent: WallpaperInstance.getStrings(), moved out of the process owner.
class Renderer
{
public:
    /// Java WallpaperInstance.start(): every setting's pre-property flags,
    /// then every setting's other arguments, both in list order. The list is
    /// Java's allSettings: global options first, then the wallpaper's own.
    static EngineCommand render(const std::string &monitor,
                                const Wallpaper &wallpaper,
                                const std::vector<const settings::Setting *> &allSettings,
                                const GpuConfig &gpu,
                                const Screenshot &screenshot,
                                const EngineOptions &options);

    /// The same, from the two containers (globals first, like Java).
    static EngineCommand render(const std::string &monitor,
                                const Wallpaper &wallpaper,
                                const settings::SettingList &settings,
                                const options::GlobalOptions &globals,
                                const GpuConfig &gpu,
                                const Screenshot &screenshot,
                                const EngineOptions &options);
};

} // namespace nyaa::domain
