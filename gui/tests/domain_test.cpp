// Plain asserts: the domain has no Qt, no event loop, no display.
#include <cassert>
#include <cstdio>

#include "domain/settings/Settings.h"

using namespace nyaa::domain::settings;

int main()
{
    BooleanSetting bloom("bloom", "Bloom", true);
    assert(bloom.toCommandArgs() == std::vector<std::string>{"--set-property", "bloom=1"});
    assert(bloom.getValueString() == "1");
    bloom.setValueFromString("false");
    assert(bloom.getValueString() == "0");
    bloom.setValueFromString("nonsense"); // must keep the current value
    assert(bloom.getValueString() == "0");
    assert(bloom.isPrePropertyFlag());

    SliderSetting fov("fov", "Field of view", 0.42, {0.1, 1.5, 0.05});
    assert(fov.toCommandArgs() == std::vector<std::string>{"--set-property", "fov=0.42"});
    fov.setValueFromString("1.234");
    assert(fov.getValueString() == "1.23");
    assert(fov.getRange().max == 1.5);

    ColorSetting tint("tint", "Tint", {1.0, 0.5333, 0.0});
    assert(tint.getValueString() == "1.0000 0.5333 0.0000");
    assert(tint.getHex() == "#FF8800");
    tint.setValueFromString("0.0 0.5 1.0");
    assert(tint.getHex() == "#0080FF");

    ComboSetting mode("mode", "Mode", 1, {{0, "day"}, {1, "night"}});
    assert(mode.toCommandArgs() == std::vector<std::string>{"--set-property", "mode=1"});
    assert(mode.getOptionLabel() == "night");
    assert(mode.getOptions().size() == 2);

    TextInputSetting note("note", "Note", "hello");
    assert(note.toCommandArgs() == std::vector<std::string>{"--set-property", "note=hello"});

    TextSetting caption("caption", "A label");
    assert(caption.toCommandArgs().empty());

    GroupSetting group("group", "Section");
    assert(group.toCommandArgs().empty());

    std::puts("domain tests passed");
    return 0;
}
