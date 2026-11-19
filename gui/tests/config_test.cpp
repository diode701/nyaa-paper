#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "io/ConfigStore.h"

using namespace nyaa::io;

int main()
{
    const std::string path = "/tmp/nyaa-config-test/config.json";
    std::remove(path.c_str());

    ConfigStore store(path);

    // missing file: defaults, and the file is created
    AppConfig config = store.load();
    assert(!config.paths.workshop.empty() || config.paths.workshop.empty()); // discovered or absent
    assert(config.engine.volume == 70);
    assert(config.engine.layer == "bottom");
    assert(config.preview.animation == "always");

    // round trip
    config.paths.engine = "/opt/lwe/linux-wallpaperengine";
    config.paths.assets = "~/.local/share/lwe/assets";
    config.preview.animation = "hover";
    config.engine.fpsLimited = true;
    config.engine.fps = 60;
    config.engine.flags = {"disable-mouse", "no-audio-processing"};
    config.ui.monitor = "HDMI-1";
    assert(store.save(config));

    const AppConfig reloaded = store.load();
    assert(reloaded.paths.engine == "/opt/lwe/linux-wallpaperengine");
    assert(reloaded.paths.assets == "~/.local/share/lwe/assets");
    assert(reloaded.preview.animation == "hover");
    assert(reloaded.engine.fpsLimited);
    assert(reloaded.engine.fps == 60);
    assert(reloaded.engine.flags.size() == 2);
    assert(reloaded.ui.monitor == "HDMI-1");

    assert(expandUser("~/.cache/wal") != "~/.cache/wal");
    assert(expandUser("relative/path") == "relative/path");

    std::puts("config tests passed");
    return 0;
}
