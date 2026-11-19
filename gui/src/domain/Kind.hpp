#pragma once

#include <string>
#include <utility>
#include <vector>

namespace nyaa::domain {

/// What a property is, stripped of everything the daemon or the engine cares
/// about. The UI switches on this to pick a delegate.
enum class Kind { Bool, Slider, Color, Combo, Text, Group, TextInput, File, Directory, Texture };

struct Rgb {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
};

struct SliderRange {
    double min = 0.0;
    double max = 1.0;
    double step = 0.01;
};

/// Everything a single project.json property can declare, reduced to
/// primitives. The JSON layer fills this in; the domain never sees a document.
struct SettingDesc {
    std::string key;
    std::string label;
    Kind kind = Kind::Text;
    std::string text;
    bool boolean = false;
    double number = 0.0;
    SliderRange range{};
    Rgb color{};
    std::vector<std::pair<int, std::string>> options;
    int order = 0;
};

} // namespace nyaa::domain
