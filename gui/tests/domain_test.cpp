// Plain asserts, no framework: the domain has no Qt, no event loop, no display.
#include <cassert>
#include <cstdio>

#include "domain/Settings.hpp"

using namespace nyaa::domain;

int main()
{
    BoolSetting bloom("bloom", "Bloom", true);
    assert(bloom.kind() == Kind::Bool);
    assert(bloom.argument() == "--set-property bloom=1");
    assert(bloom.serialize() == "1");
    bloom.restore("false");
    assert(bloom.argument() == "--set-property bloom=0");
    assert(bloom.display() == "off");
    bloom.restore("nonsense"); // must keep the current value
    assert(bloom.serialize() == "0");
    assert(bloom.emitsArgument());

    SliderSetting fov("fov", "Field of view", 0.42, {0.1, 1.5, 0.05});
    assert(fov.argument() == "--set-property fov=0.42");
    fov.restore("1.234");
    assert(fov.serialize() == "1.23");
    assert(fov.range().max == 1.5);

    ColorSetting tint("tint", "Tint", {1.0, 0.5333, 0.0});
    assert(tint.argument() == "--set-property tint=1.0000 0.5333 0.0000");
    assert(tint.display() == "#FF8800");
    tint.restore("0.0 0.5 1.0");
    assert(tint.display() == "#0080FF");

    ComboSetting mode("mode", "Mode", 1, {{0, "day"}, {1, "night"}});
    assert(mode.argument() == "--set-property mode=1");
    assert(mode.display() == "night");
    assert(mode.options().size() == 2);

    TextInputSetting note("note", "Note", "hello");
    assert(note.argument() == "--set-property note=hello");
    assert(note.serialize() == "hello");

    TextSetting caption("caption", "A label");
    assert(caption.argument().empty());
    assert(!caption.emitsArgument());

    GroupSetting group("group", "Section");
    assert(!group.emitsArgument());

    std::puts("domain tests passed");
    return 0;
}
