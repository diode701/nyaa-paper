#include "domain/settings/SettingFactory.h"

#include <utility>

#include "domain/settings/Settings.h"

namespace nyaa::domain::settings {

std::unique_ptr<Setting> makeSetting(const SettingDesc &desc)
{
    switch (desc.kind) {
    case Kind::Bool:
        return std::make_unique<BooleanSetting>(desc.key, desc.label, desc.boolean);
    case Kind::Slider:
        return std::make_unique<SliderSetting>(desc.key, desc.label, desc.number, desc.range);
    case Kind::Color:
        return std::make_unique<ColorSetting>(desc.key, desc.label, desc.color);
    case Kind::Combo:
        return std::make_unique<ComboSetting>(desc.key, desc.label,
                                              static_cast<int>(desc.number), desc.options);
    case Kind::Text:
        return std::make_unique<TextSetting>(desc.key, desc.label);
    case Kind::Group:
        return std::make_unique<GroupSetting>(desc.key, desc.label);
    case Kind::TextInput:
        return std::make_unique<TextInputSetting>(desc.key, desc.label, desc.text);
    case Kind::File:
        return std::make_unique<FileSetting>(desc.key, desc.label, desc.text);
    case Kind::Directory:
        return std::make_unique<DirectorySetting>(desc.key, desc.label, desc.text);
    case Kind::Texture:
        return std::make_unique<TextureSetting>(desc.key, desc.label, desc.text);
    }
    return nullptr; // unknown type: the parser already logged it
}

} // namespace nyaa::domain::settings
