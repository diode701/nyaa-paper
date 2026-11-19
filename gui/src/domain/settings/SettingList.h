#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "domain/settings/Setting.h"

namespace nyaa::domain::settings {

/// Owns the settings of one wallpaper. Java equivalent: the
/// List<Setting> that lived in WallpaperPropertiesView, plus its save/load loops.
class SettingList
{
public:
    void add(std::unique_ptr<Setting> setting);

    /// Null when the wallpaper has no such property.
    Setting *find(const std::string &name) const;

    const std::vector<std::unique_ptr<Setting>> &all() const { return settings_; }

    /// Every argument fragment, in insertion order.
    std::vector<std::string> arguments(bool prePropertyFlags) const;

    /// name -> getValueString(), for the config file.
    std::map<std::string, std::string> serialize() const;

    /// Applies whatever matches by name; unknown keys are ignored.
    void restore(const std::map<std::string, std::string> &saved);

private:
    std::vector<std::unique_ptr<Setting>> settings_;
};

} // namespace nyaa::domain::settings
