#pragma once

#include "WallpaperModel.hpp"
#include <QVector>

/// Stand-in for the daemon's Library interface, so the UI can run before the
/// client is wired to D-Bus. Reads the real workshop folder when one exists.
namespace MockLibrary {
QVector<nyaa::domain::Wallpaper> load();
QVector<nyaa::domain::Wallpaper> fromWorkshop(const QString &root);
QVector<nyaa::domain::Wallpaper> sample();
}
