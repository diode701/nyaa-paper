#pragma once

#include <set>
#include <string>

#include "domain/WallpaperConfig.h"

namespace nyaa::io {

/// The last applied WallpaperConfig per monitor, one record per monitor.
///
/// Java equivalent: WallpaperConfigManager (one file per monitor directory,
/// saving a new one deleted the old). Here it is one JSON file keyed by
/// monitor; the record keeps Java's field names.
class WallpaperStore
{
public:
    WallpaperStore();
    explicit WallpaperStore(std::string path);

    /// Empty record when nothing was saved for this monitor.
    domain::WallpaperConfig load(const std::string &monitor) const;
    /// Replaces whatever that monitor had (config.monitor is the key).
    bool save(const domain::WallpaperConfig &config) const;
    /// WallpaperConfigManager.delete
    bool remove(const std::string &monitor) const;
    /// WallpaperConfigManager.cleanupStale: drops monitors whose wallpaper is
    /// no longer subscribed. Returns how many records were removed.
    int cleanupStale(const std::set<std::string> &activeIds) const;

    static std::string defaultPath();

private:
    std::string path_;
};

} // namespace nyaa::io
