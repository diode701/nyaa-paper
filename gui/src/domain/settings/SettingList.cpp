#include "domain/settings/SettingList.h"

#include <utility>

namespace nyaa::domain::settings {

void SettingList::add(std::unique_ptr<Setting> setting)
{
    if (setting)
        settings_.push_back(std::move(setting));
}

Setting *SettingList::find(const std::string &name) const
{
    for (const auto &setting : settings_) {
        if (setting->getName() == name)
            return setting.get();
    }
    return nullptr;
}

std::vector<std::string> SettingList::arguments(bool prePropertyFlags) const
{
    std::vector<std::string> out;
    for (const auto &setting : settings_) {
        if (setting->isPrePropertyFlag() != prePropertyFlags)
            continue;
        const std::vector<std::string> args = setting->toCommandArgs();
        out.insert(out.end(), args.begin(), args.end());
    }
    return out;
}

std::map<std::string, std::string> SettingList::serialize() const
{
    std::map<std::string, std::string> out;
    // Java's captureConfig saves every non-null value; only labels and group
    // headers return null. An empty text input is a real value.
    for (const auto &setting : settings_) {
        if (setting->getKind() == Kind::Text || setting->getKind() == Kind::Group)
            continue;
        out[setting->getName()] = setting->getValueString();
    }
    return out;
}

void SettingList::restore(const std::map<std::string, std::string> &saved)
{
    for (const auto &[name, value] : saved) {
        if (Setting *setting = find(name))
            setting->setValueFromString(value);
    }
}

} // namespace nyaa::domain::settings
