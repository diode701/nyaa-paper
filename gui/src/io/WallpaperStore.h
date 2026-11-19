#pragma once

#include <map>
#include <string>

namespace nyaa::io {

/// Remembers what was last applied on each monitor, and with which values, so
/// "Load last" can put the sliders back without applying anything.
class WallpaperStore
{
public:
    struct Last {
        std::string wallpaperId;
        std::map<std::string, std::string> values;

        bool empty() const { return wallpaperId.empty(); }
    };

    WallpaperStore();
    explicit WallpaperStore(std::string path);

    Last load(const std::string &monitor) const;
    bool save(const std::string &monitor, const Last &last) const;

    static std::string defaultPath();

private:
    std::string path_;
};

} // namespace nyaa::io
