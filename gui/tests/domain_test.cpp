// Plain asserts: the domain has no Qt, no event loop, no display.
// The checks must run in every build type, Release (NDEBUG) included.
#undef NDEBUG
#include <cassert>
// Variadic so braced initialisers with commas stay one argument.
#define CHECK(...) assert((__VA_ARGS__))
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
    CHECK(bloom.toCommandArgs() == std::vector<std::string>{"--set-property", "bloom=1"});
    CHECK(bloom.getValueString() == "1");
    bloom.setValueFromString("false");
    CHECK(bloom.getValueString() == "0");
    bloom.setValueFromString("nonsense"); // must keep the current value
    CHECK(bloom.getValueString() == "0");
    CHECK(!bloom.isPrePropertyFlag()); // properties travel after the engine flags

    SliderSetting fov("fov", "Field of view", 0.42, {0.1, 1.5, 0.05});
    CHECK(fov.toCommandArgs() == std::vector<std::string>{"--set-property", "fov=0.42"});
    fov.setValueFromString("1.234");
    CHECK(fov.getValueString() == "1.23");
    CHECK(fov.getRange().max == 1.5);

    ColorSetting tint("tint", "Tint", {1.0, 0.5333, 0.0});
    CHECK(tint.getValueString() == "1.0000 0.5333 0.0000");
    CHECK(tint.getHex() == "#FF8800");
    tint.setValueFromString("0.0 0.5 1.0");
    CHECK(tint.getHex() == "#0080FF");

    ComboSetting mode("mode", "Mode", 1, {{0, "day"}, {1, "night"}});
    CHECK(mode.toCommandArgs() == std::vector<std::string>{"--set-property", "mode=1"});
    CHECK(mode.getOptionLabel() == "night");
    CHECK(mode.getOptions().size() == 2);

    TextInputSetting note("note", "Note", "hello");
    CHECK(note.toCommandArgs() == std::vector<std::string>{"--set-property", "note=hello"});

    TextSetting caption("caption", "A label");
    CHECK(caption.toCommandArgs().empty());

    GroupSetting group("group", "Section");
    CHECK(group.toCommandArgs().empty());

    // factory: a desc becomes the right subclass
    SettingDesc desc;
    desc.key = "fov";
    desc.label = "FOV";
    desc.kind = Kind::Slider;
    desc.number = 0.42;
    desc.range = {0.1, 1.5, 0.05};
    auto built = makeSetting(desc);
    CHECK(built && built->getName() == "fov");
    CHECK(built->toCommandArgs() == std::vector<std::string>{"--set-property", "fov=0.42"});

    // renderer: the exact argv the daemon will run
    SettingList list;
    list.add(std::make_unique<BooleanSetting>("bloom", "Bloom", true));
    list.add(std::make_unique<SliderSetting>("fov", "FOV", 0.42, SliderRange{0.0, 1.0, 0.01}));
    list.add(std::make_unique<TextSetting>("caption", "A label"));

    Wallpaper wallpaper{"1234", "Test", "scene", "scene.pkg", "/tmp/1234", ""};
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
    CHECK(command.argv == expected);
    CHECK(command.env == std::vector<std::string>{"DRI_PRIME=1", "__GL_THREADED_OPTIMIZATIONS=0"});
    CHECK(command.wallpaperId == "1234");

    // a wallpaper outside the workshop root is passed by path, not by id
    nyaa::domain::Wallpaper odd{"9999", "Odd", "scene", "x.pkg", "/elsewhere/9999", ""};
    const EngineCommand byPath = nyaa::domain::Renderer::render("eDP-1", odd, list, globals, gpu, {}, options);
    CHECK(byPath.argv[3] == "/elsewhere/9999");

    // K2: a GlobalOption IS a Setting (Java: GlobalOption extends Setting), so
    // one list of Setting* renders exactly like the two containers.
    {
        nyaa::domain::options::GlobalOptions g;
        g.volume().setMuted(true);
        g.setFlag("disable-mouse", true);
        g.clamp().setEnabled(true);
        std::vector<const Setting *> all;
        for (const auto *option : g.all())
            all.push_back(option);                 // upcast: no adapter needed
        for (const auto &setting : list.all())
            all.push_back(setting.get());
        CHECK(all.front()->getKind() == Kind::Volume);
        CHECK(all.front()->getDescription() == "Audio");
        const auto one = nyaa::domain::Renderer::render("eDP-1", wallpaper, all, gpu, {}, options);
        const auto two = nyaa::domain::Renderer::render("eDP-1", wallpaper, list, g, gpu, {}, options);
        CHECK(one.argv == two.argv);
        const std::vector<std::string> tail(one.argv.begin() + 8, one.argv.end());
        CHECK((tail == std::vector<std::string>{"--silent", "--disable-mouse", "--clamp", "border",
                                                  "--set-property", "bloom=1", "--set-property", "fov=0.42"}));
        g.reset();
        CHECK(!g.volume().isMuted() && g.arguments(true) == std::vector<std::string>({"--volume", "70"}));
    }

    std::puts("domain tests passed");
    return 0;
}
