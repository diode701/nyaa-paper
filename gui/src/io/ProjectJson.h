#pragma once

#include <string>
#include <vector>

#include "domain/settings/Kind.h"

namespace nyaa::io {

/// Reads a wallpaper's project.json and returns one descriptor per property,
/// in the order the author declared. Unknown or malformed entries are skipped:
/// one bad property must never hide the other 40.
std::vector<nyaa::domain::settings::SettingDesc> parseProperties(const std::string &json);

/// The same, from a file; an unreadable or invalid file yields an empty list.
std::vector<nyaa::domain::settings::SettingDesc> parseProjectFile(const std::string &path);

} // namespace nyaa::io
