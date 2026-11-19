// Plain asserts, no framework: the domain has no Qt, no event loop, no display.
#include <cassert>
#include <cstdio>

#include "domain/BoolSetting.hpp"
#include "domain/ColorSetting.hpp"
#include "domain/SliderSetting.hpp"

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

    SliderSetting fov("fov", "Field of view", 0.42, {0.1, 1.5, 0.05});
    assert(fov.kind() == Kind::Slider);
    assert(fov.argument() == "--set-property fov=0.42");
    fov.restore("1.234");
    assert(fov.serialize() == "1.23");
    assert(fov.range().max == 1.5);
    fov.restore("not a number");
    assert(fov.serialize() == "1.23");

    ColorSetting tint("tint", "Tint", {1.0, 0.5333, 0.0});
    assert(tint.kind() == Kind::Color);
    assert(tint.argument() == "--set-property tint=1.0000 0.5333 0.0000");
    assert(tint.display() == "#FF8800");
    tint.restore("0.0 0.5 1.0");
    assert(tint.display() == "#0080FF");

    std::puts("domain tests passed");
    return 0;
}
