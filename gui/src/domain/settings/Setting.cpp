#include "domain/settings/Setting.h"

namespace nyaa::domain::settings {

Setting::Setting(const std::string &name, const std::string &description)
    : name_(name)
    , description_(description)
{
}

std::string Setting::getName() const { return name_; }

std::string Setting::getDescription() const { return description_; }

} // namespace nyaa::domain::settings
