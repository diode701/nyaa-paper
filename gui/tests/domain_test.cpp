// Plain asserts: the domain has no Qt, no event loop, no display.
#include <cassert>
#include <cstdio>

#include "domain/options/GlobalOptions.h"
#include "domain/Renderer.h"
#include "domain/settings/SettingFactory.h"
#include "domain/settings/SettingList.h"
#include "domain/settings/Settings.h"

using namespace nyaa::domain;
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
    assert(!bloom.isPrePropertyFlag()); // properties travel after the engine flags

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

    // factory: a desc becomes the right subclass
    SettingDesc desc;
    desc.key = "fov";
    desc.label = "FOV";
    desc.kind = Kind::Slider;
    desc.number = 0.42;
    desc.range = {0.1, 1.5, 0.05};
    auto built = makeSetting(desc);
    assert(built && built->getName() == "fov");
    assert(built->toCommandArgs() == std::vector<std::string>{"--set-property", "fov=0.42"});

    // renderer: the exact argv the daemon will run
    SettingList list;
    list.add(std::make_unique<BooleanSetting>("bloom", "Bloom", true));
    list.add(std::make_unique<SliderSetting>("fov", "FOV", 0.42, SliderRange{0.0, 1.0, 0.01}));
    list.add(std::make_unique<TextSetting>("caption", "A label"));

    Wallpaper wallpaper{"1234", "Test", "scene", "scene.pkg", "/tmp/1234"};
    GpuConfig gpu;
    gpu.mode = GpuConfig::Mode::DriPrime;
    gpu.driPrimeNumber = 1;
    gpu.glThreaded = true;

    nyaa::domain::options::GlobalOptions globals;   // volume 70 default, no flags
    globals.restore({{"volume", "70"}});
    nyaa::domain::EngineOptions options;
    options.workshopRoot = "/tmp";             // basePath /tmp/1234 is under it -> id
    options.assetsDir = "/opt/assets";
    options.layer = "bottom";
    const EngineCommand command = nyaa::domain::Renderer::render("eDP-1", wallpaper, list, globals, gpu, {}, options);
    const std::vector<std::string> expected{
        "--screen-root", "eDP-1", "--bg", "1234",
        "--assets-dir", "/opt/assets",
        "--layer", "bottom",
        "--volume", "70",
        "--set-property", "bloom=1",
        "--set-property", "fov=0.42",
    };
    assert(command.argv == expected);
    assert(command.env == std::vector<std::string>{"DRI_PRIME=1", "__GL_THREADED_OPTIMIZATIONS=0"});
    assert(command.wallpaperId == "1234");

    // a wallpaper outside the workshop root is passed by path, not by id
    nyaa::domain::Wallpaper odd{"9999", "Odd", "scene", "x.pkg", "/elsewhere/9999"};
    const EngineCommand byPath = nyaa::domain::Renderer::render("eDP-1", odd, list, globals, gpu, {}, options);
    assert(byPath.argv[3] == "/elsewhere/9999");

    std::puts("domain tests passed");
    return 0;
}
