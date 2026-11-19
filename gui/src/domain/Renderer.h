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
    static EngineCommand render(const std::string &monitor,
                                const Wallpaper &wallpaper,
                                const settings::SettingList &settings,
                                const options::GlobalOptions &globals,
                                const GpuConfig &gpu,
                                const Screenshot &screenshot,
                                const EngineOptions &options);
};

} // namespace nyaa::domain
