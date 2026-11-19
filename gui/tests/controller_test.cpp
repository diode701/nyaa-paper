// PropertiesController against the mock engine: what you edit is what runs,
// and every monitor keeps its own wallpaper, options and GPU (docs/TODO.md C).
#include <cassert>
#include <cstdio>

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "bridge/ConfigBridge.hpp"
#include "bridge/EngineBridge.hpp"
#include "bridge/PlaylistBridge.hpp"
#include "app/StartupRestore.hpp"
#include "bridge/PropertiesController.hpp"
#include "io/WallpaperStore.h"
#include "models/GlobalOptionsModel.hpp"
#include "models/PropertyModel.hpp"
#include "models/WallpaperModel.hpp"

using nyaa::domain::GpuConfig;

namespace {
bool has(const QStringList &list, const QString &value) { return list.contains(value); }

bool hasPair(const QStringList &argv, const QString &flag, const QString &value)
{
    const int i = argv.indexOf(flag);
    return i >= 0 && i + 1 < argv.size() && argv[i + 1] == value;
}

void spinFor(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

/// Run the event loop until the controller reports `expected`.
bool waitForStatus(PropertiesController &controller, const QString &expected)
{
    QString last;
    auto connection = QObject::connect(&controller, &PropertiesController::status,
                                       [&last](const QString &m) { last = m; });
    QElapsedTimer timer;
    timer.start();
    while (last != expected && timer.elapsed() < 2000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    QObject::disconnect(connection);
    if (last != expected)
        std::fprintf(stderr, "status was '%s', expected '%s'\n", qPrintable(last), qPrintable(expected));
    return last == expected;
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    // NYAA_TEST_DAEMON=1 runs against a real nyaa-paperd (scripts/e2e-gpu.sh
    // starts one on a private bus with a fake engine); otherwise the mock.
    if (!qEnvironmentVariableIsSet("NYAA_TEST_DAEMON"))
        qputenv("NYAA_MOCK", "1");
    QStandardPaths::setTestModeEnabled(true);   // never touch the real ~/.config
    QGuiApplication app(argc, argv);

    QFile::remove(QString::fromStdString(nyaa::io::WallpaperStore::defaultPath()));

    QTemporaryDir wallpaperDir;
    {
        QFile project(wallpaperDir.filePath("project.json"));
        assert(project.open(QIODevice::WriteOnly));
        project.write(R"({ "title": "test", "type": "scene", "general": { "properties": {
            "fov":   { "type": "slider", "text": "FOV", "value": 0.5, "min": 0, "max": 1, "step": 0.1, "order": 1 },
            "bloom": { "type": "bool",   "text": "Bloom", "value": false, "order": 2 },
            "mode":  { "type": "combo",  "text": "Mode", "value": "0", "order": 3,
                       "options": [ {"label": "day", "value": "0"}, {"label": "night", "value": "1"} ] }
        } } })");
    }

    ConfigBridge config;
    EngineBridge engine;
    WallpaperModel library;
    PlaylistBridge playlist(library, config);
    PropertyModel properties;
    GlobalOptionsModel globals;
    PropertiesController controller(properties, globals, engine, playlist, config, library);

    // ---- open: project.json defaults, fresh global options, default GPU ----
    assert(controller.open("777", wallpaperDir.path()) == 3);
    EngineRequest r = controller.render("TEST-1", false);
    assert(has(r.argv, "fov=0.50"));
    assert(has(r.argv, "bloom=0"));
    assert(has(r.argv, "mode=0"));
    assert(hasPair(r.argv, "--volume", "70"));
    assert(r.env.isEmpty());

    // ---- edit everything a user can touch --------------------------------
    properties.setValue("fov", 0.9);
    properties.setValue("bloom", true);
    properties.setValue("mode", "night");          // the combo delegate sends the label
    globals.setValue("volume", 30);
    globals.setValue("disable-mouse", true);
    globals.setValue("clamp", true);               // B3: clamping can be enabled
    globals.setValue("clamp", QStringLiteral("repeat"));
    controller.setGpuMode("DRI_PRIME");
    controller.setDriPrimeNumber(2);

    // C3: the command is built from the edits, not from project.json
    r = controller.render("TEST-1", false);
    assert(has(r.argv, "fov=0.90"));
    assert(has(r.argv, "bloom=1"));
    assert(has(r.argv, "mode=1"));
    assert(hasPair(r.argv, "--volume", "30"));
    assert(has(r.argv, "--disable-mouse"));
    assert(hasPair(r.argv, "--clamp", "repeat"));
    assert(has(r.env, "DRI_PRIME=2"));

    // C2: capture holds custom values, global values and the GPU
    const auto captured = controller.capture("TEST-1");
    assert(captured.settings.at("fov") == "0.90");
    assert(captured.settings.at("volume") == "30");
    assert(captured.settings.at("disable-mouse") == "true");
    assert(captured.settings.at("clamp") == "true|repeat");
    assert(captured.gpu.mode == GpuConfig::Mode::DriPrime && captured.gpu.driPrimeNumber == 2);

    // C5: status follows the engine's answer; the config is saved on success
    controller.apply("TEST-1");
    assert(waitForStatus(controller, "Applied to TEST-1"));

    // ---- a second monitor, different GPU, same wallpaper ------------------
    controller.setGpuMode("NVIDIA");
    controller.apply("TEST-2");
    assert(waitForStatus(controller, "Applied to TEST-2"));

    nyaa::io::WallpaperStore store;
    const auto one = store.current("TEST-1");
    const auto two = store.current("TEST-2");
    assert(one.gpu.mode == GpuConfig::Mode::DriPrime && one.gpu.driPrimeNumber == 2);
    assert(two.gpu.mode == GpuConfig::Mode::Nvidia);   // per monitor, per GPU
    assert(one.settings.at("fov") == "0.90");

    // ---- reopen: back to defaults, like a new Java properties view --------
    controller.open("777", wallpaperDir.path());
    r = controller.render("TEST-1", false);
    assert(has(r.argv, "fov=0.50"));
    assert(hasPair(r.argv, "--volume", "70"));
    assert(controller.gpuMode() == "DEFAULT");

    // C6: Load last restores custom and global values (GPU untouched, as in Java)
    controller.loadLast("TEST-1");
    r = controller.render("TEST-1", false);
    assert(has(r.argv, "fov=0.90"));
    assert(has(r.argv, "bloom=1"));
    assert(hasPair(r.argv, "--volume", "30"));
    assert(has(r.argv, "--disable-mouse"));
    assert(controller.gpuMode() == "DEFAULT");

    // a different wallpaper has nothing saved on that monitor
    controller.open("888", wallpaperDir.path());
    QString message;
    QObject::connect(&controller, &PropertiesController::status, [&message](const QString &m) { message = m; });
    controller.loadLast("TEST-1");
    assert(message == "No saved settings for this wallpaper on TEST-1");

    // C8: apply 888 on TEST-1 too, then go back to 777. Java had deleted 777's
    // settings when 888 was saved; here Load last still finds them.
    properties.setValue("fov", 0.2);
    controller.apply("TEST-1");
    assert(waitForStatus(controller, "Applied to TEST-1"));
    assert(store.current("TEST-1").wallpaperId == "888");
    controller.open("777", wallpaperDir.path());
    controller.loadLast("TEST-1");
    assert(message == "Loaded 14 saved settings for TEST-1");
    r = controller.render("TEST-1", false);
    assert(has(r.argv, "fov=0.90"));
    controller.open("888", wallpaperDir.path());
    controller.loadLast("TEST-1");
    assert(has(controller.render("TEST-1", false).argv, "fov=0.20"));

    // the playlist entry is rendered from the edits too, without a screenshot
    properties.setValue("fov", 0.3);
    controller.setScreenshotEnabled(true);
    const QVariantMap entry = controller.playlistEntry("TEST-1");
    const QStringList entryArgv = entry.value("argv").toStringList();
    assert(has(entryArgv, "fov=0.30"));
    assert(!has(entryArgv, "--screenshot"));

    // ---- J1: autoRestoreWallpapers ------------------------------------------
    {
        library.setAll({WallpaperModel::make("777", "test", "scene", {}, wallpaperDir.path())});
        engine.setCustomMonitors({"TEST-R", "TEST-SKIP"});

        auto saved = controller.capture("TEST-R");
        saved.wallpaperId = "777";   // the one in this library
        saved.gpu.mode = GpuConfig::Mode::DriPrime;
        store.save(saved);
        // a monitor the daemon already runs must be left alone
        auto running = controller.capture("TEST-SKIP");
        running.wallpaperId = "777";
        store.save(running);

        QStringList applied;
        auto watch = QObject::connect(&engine, &EngineBridge::applyFinished,
                                      [&applied](const QString &m, bool ok, const QString &) {
                                          if (ok) applied << m;
                                      });
        QStringList restoredMonitors;
        bool reported = false;
        {
            StartupRestore restore(config, engine, library);
            QObject::connect(&restore, &StartupRestore::restored, [&](const QStringList &m) {
                restoredMonitors = m;
                reported = true;
            });
            restore.libraryLoaded();
            QElapsedTimer t;
            t.start();
            while (!reported && t.elapsed() < 2000)
                QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
            spinFor(200);
        }
        assert(reported);
        assert(restoredMonitors.contains("TEST-R"));
        assert(applied.contains("TEST-R"));
        QObject::disconnect(watch);
    }

    std::puts("controller tests passed");
    return 0;
}
