#pragma once

#include <QJsonObject>

#include "domain/WallpaperConfig.h"

namespace nyaa::io {

/// WallpaperConfig.toJson / fromJson, with Java's field names and defaults.
/// Shared by the per-monitor store and by playlist entries, which hold the
/// same record (Java: Playlist.entries is a List<WallpaperConfig>).
QJsonObject toJson(const domain::WallpaperConfig &config);
domain::WallpaperConfig fromJson(const QJsonObject &json);

} // namespace nyaa::io
