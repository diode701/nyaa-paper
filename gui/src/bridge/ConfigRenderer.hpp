#pragma once

#include <QString>

#include "domain/WallpaperConfig.h"
#include "engine/IEngine.hpp"
#include "io/AppConfig.h"

/// Turns a saved WallpaperConfig back into the exact engine command.
///
/// Java: PlaylistRunner.buildSettingsFromConfig + WallpaperInstance. The
/// wallpaper's project.json gives the settings and their defaults, the saved
/// values go on top, then the global options and the GPU. Used for playlist
/// entries now, and for startup restore later.
namespace ConfigRenderer {
EngineRequest render(const nyaa::domain::WallpaperConfig &config, const QString &basePath,
                     const nyaa::io::AppConfig &app);
}
