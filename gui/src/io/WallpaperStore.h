#pragma once

#include <set>
#include <string>

#include "domain/WallpaperConfig.h"

namespace nyaa::io {

/// Saved WallpaperConfigs: per monitor, per wallpaper, plus which one is the
/// monitor's current wallpaper.
///
/// Java's WallpaperConfigManager kept ONE file per monitor and deleted the
/// others on every save, so "Load last" only worked for the wallpaper applied
/// most recently on that monitor. Here every wallpaper keeps its own config on
/// every monitor; `current` is what startup restore puts back.
///
///   { "eDP-1": { "current": "1234",
///                "configs": { "1234": {...}, "5678": {...} } } }
class WallpaperStore
{
public:
    WallpaperStore();
    explicit WallpaperStore(std::string path);

    /// The config this wallpaper last ran with on this monitor (Load last).
    /// Empty when it never ran there.
    domain::WallpaperConfig load(const std::string &monitor, const std::string &wallpaperId) const;
    /// What the monitor ran last (startup restore). Empty when nothing.
    domain::WallpaperConfig current(const std::string &monitor) const;
    /// Stores the config under its monitor and wallpaper and makes it current.
    bool save(const domain::WallpaperConfig &config) const;
    /// WallpaperConfigManager.delete(monitor, id)
    bool remove(const std::string &monitor, const std::string &wallpaperId) const;
    /// Drops configs of wallpapers that are no longer subscribed. An empty set
    /// means "the library could not be read", never "unsubscribed from
    /// everything", so it removes nothing. Returns how many were removed.
    int cleanupStale(const std::set<std::string> &activeIds) const;

    static std::string defaultPath();

private:
    std::string path_;
};

} // namespace nyaa::io
