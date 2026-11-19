#pragma once

#include <memory>

#include "domain/settings/Kind.h"

namespace nyaa::domain::settings {

class Setting;

/// The only place that knows all ten concrete kinds. Everything else sees
/// Setting pointers and virtual dispatch.
std::unique_ptr<Setting> makeSetting(const SettingDesc &desc);

} // namespace nyaa::domain::settings
