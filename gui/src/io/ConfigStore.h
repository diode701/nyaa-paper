#pragma once

#include <string>

#include "io/AppConfig.h"

namespace nyaa::io {

/// Loads and saves config.json. The path is injected so tests never touch the
/// real user config; the no-argument constructor uses the standard location.
class ConfigStore
{
public:
    ConfigStore();
    explicit ConfigStore(std::string path);

    /// Absolute path of the file this store reads and writes.
    const std::string &path() const { return path_; }

    /// Missing file: returns defaults and writes them once. Malformed file:
    /// returns defaults and leaves the broken file alone (it is evidence).
    AppConfig load() const;

    /// Atomic-ish write: temp file in the same directory, then rename.
    bool save(const AppConfig &config) const;

    /// The standard location: $XDG_CONFIG_HOME/nyaa-paper/config.json
    static std::string defaultPath();

    /// First existing Steam workshop folder, native or flatpak; empty if none.
    static std::string detectWorkshopPath();

private:
    std::string path_;
};

/// "~" and "$HOME" expanded, nothing else.
std::string expandUser(const std::string &path);

} // namespace nyaa::io
