#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <QFile>

#include "io/ConfigStore.h"
#include "io/WallpaperStore.h"

using namespace nyaa::io;
using nyaa::domain::GpuConfig;
using nyaa::domain::WallpaperConfig;

int main()
{
    const std::string path = "/tmp/nyaa-config-test/config.json";
    std::remove(path.c_str());

    ConfigStore store(path);

    // missing file: defaults, and the file is created
    AppConfig config = store.load();
    assert(config.engine.layer == "bottom");
    assert(config.preview.animation == "always");

    // round trip
    config.paths.engine = "/opt/lwe/linux-wallpaperengine";
    config.paths.assets = "~/.local/share/lwe/assets";
    config.preview.animation = "hover";
    config.engine.layer = "background";
    config.ui.monitor = "HDMI-1";
    assert(store.save(config));

    const AppConfig reloaded = store.load();
    assert(reloaded.paths.engine == "/opt/lwe/linux-wallpaperengine");
    assert(reloaded.paths.assets == "~/.local/share/lwe/assets");
    assert(reloaded.preview.animation == "hover");
    assert(reloaded.engine.layer == "background");
    assert(reloaded.ui.monitor == "HDMI-1");

    assert(expandUser("~/.cache/wal") != "~/.cache/wal");
    assert(expandUser("relative/path") == "relative/path");

    // ---- WallpaperConfig per monitor: each monitor its own GPU (TODO C1) ----
    const std::string wpPath = "/tmp/nyaa-config-test/wallpapers.json";
    std::remove(wpPath.c_str());
    WallpaperStore wallpapers(wpPath);

    WallpaperConfig left;
    left.wallpaperId = "1234";
    left.monitor = "eDP-1";
    left.projectJsonModified = 1700000000123LL;
    left.settings["bloom"] = "1";
    left.settings["volume"] = "silent|40";
    left.gpu.mode = GpuConfig::Mode::DriPrime;
    left.gpu.driPrimeNumber = 2;
    assert(wallpapers.save(left));

    WallpaperConfig right;
    right.wallpaperId = "5678";
    right.monitor = "HDMI-1";
    right.gpu.mode = GpuConfig::Mode::Nvidia;
    right.gpu.glThreaded = true;
    assert(wallpapers.save(right));

    const WallpaperConfig a = wallpapers.load("eDP-1");
    assert(a.wallpaperId == "1234" && a.monitor == "eDP-1");
    assert(a.projectJsonModified == 1700000000123LL);
    assert(a.settings.at("bloom") == "1");
    assert(a.settings.at("volume") == "silent|40");
    assert(a.gpu.mode == GpuConfig::Mode::DriPrime && a.gpu.driPrimeNumber == 2 && !a.gpu.glThreaded);

    const WallpaperConfig b = wallpapers.load("HDMI-1");
    assert(b.wallpaperId == "5678");
    assert(b.gpu.mode == GpuConfig::Mode::Nvidia && b.gpu.glThreaded);   // independent of eDP-1
    assert(wallpapers.load("DP-3").empty());

    // saving again replaces only that monitor (Java: one config per monitor)
    left.wallpaperId = "9999";
    assert(wallpapers.save(left));
    assert(wallpapers.load("eDP-1").wallpaperId == "9999");
    assert(wallpapers.load("HDMI-1").wallpaperId == "5678");

    // cleanupStale drops monitors whose wallpaper is no longer subscribed (TODO C7)
    assert(wallpapers.cleanupStale({"5678"}) == 1);
    assert(wallpapers.load("eDP-1").empty());
    assert(!wallpapers.load("HDMI-1").empty());

    // delete
    assert(wallpapers.remove("HDMI-1"));
    assert(wallpapers.load("HDMI-1").empty());

    // files written by the first v2 builds ("values") still load
    {
        QFile legacy(QString::fromStdString(wpPath));
        assert(legacy.open(QIODevice::WriteOnly));
        legacy.write(R"({"eDP-1": {"wallpaperId": "42", "values": {"bloom": "0"}}})");
    }
    const WallpaperConfig old = wallpapers.load("eDP-1");
    assert(old.wallpaperId == "42" && old.settings.at("bloom") == "0");
    assert(old.gpu.mode == GpuConfig::Mode::Default);

    std::puts("config tests passed");
    return 0;
}
