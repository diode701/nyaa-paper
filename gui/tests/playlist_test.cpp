// PlaylistBridge against a real nyaa-paperd (run by scripts/e2e.sh on a
// private bus). docs/TODO.md group D.
#include <cassert>
#include <cstdio>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>

#include "bridge/ConfigBridge.hpp"
#include "bridge/PlaylistBridge.hpp"
#include "io/WallpaperConfigJson.h"
#include "models/WallpaperModel.hpp"

namespace {

void spin(int ms)
{
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

/// The stored document, straight from the daemon (not through the bridge).
QJsonObject stored(const QString &monitor)
{
    QDBusInterface daemon("org.nyaa.Paper", "/org/nyaa/Paper", "org.nyaa.Paper.Playlist");
    const QDBusReply<QString> reply = daemon.call("Get", monitor);
    assert(reply.isValid());
    return QJsonDocument::fromJson(reply.value().toUtf8()).object();
}

QStringList listed()
{
    QDBusInterface daemon("org.nyaa.Paper", "/org/nyaa/Paper", "org.nyaa.Paper.Playlist");
    const QDBusReply<QStringList> reply = daemon.call("ListMonitors");
    assert(reply.isValid());
    return reply.value();
}

/// An entry the way PropertiesController.playlistEntry builds it.
QVariantMap entryFor(const QString &id, const QString &fov, nyaa::domain::GpuConfig::Mode gpu)
{
    nyaa::domain::WallpaperConfig config;
    config.wallpaperId = id.toStdString();
    config.settings = {{"fov", fov.toStdString()}, {"volume", "40"}};
    config.gpu.mode = gpu;
    config.gpu.driPrimeNumber = 2;
    QVariantMap entry;
    entry["wallpaperId"] = id;
    entry["config"] = nyaa::io::toJson(config).toVariantMap();
    entry["argv"] = QStringList{};   // the bridge renders it
    entry["durationSec"] = 0.0;
    return entry;
}

bool argvHas(const QJsonObject &entry, const QString &value)
{
    return entry.value("argv").toArray().contains(QJsonValue(value));
}

} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QStandardPaths::setTestModeEnabled(true);
    QGuiApplication app(argc, argv);
    assert(QDBusConnection::sessionBus().isConnected());

    // Two wallpapers in the library, each with a project.json.
    QTemporaryDir dir;
    QVector<WallpaperItem> items;
    for (const QString &id : {QStringLiteral("777"), QStringLiteral("888")}) {
        const QString base = dir.filePath(id);
        QDir().mkpath(base);
        QFile project(base + "/project.json");
        assert(project.open(QIODevice::WriteOnly));
        project.write(R"({ "title": "t", "type": "scene", "general": { "properties": {
            "fov": { "type": "slider", "value": 0.5, "min": 0, "max": 1, "step": 0.1 } } } })");
        items.push_back({id, "title " + id, "scene", {}, base});
    }
    WallpaperModel library;
    library.setAll(items);
    ConfigBridge config;
    PlaylistBridge playlist(library, config);
    playlist.setConnectedMonitors({"CONNECTED-1"});

    // ---- D2: add to the monitor asked for, not the one the views show ----
    playlist.load("VIEW-0");
    assert(playlist.addTo("TEST-1", entryFor("777", "0.90", nyaa::domain::GpuConfig::Mode::DriPrime)) == 1);
    assert(playlist.addTo("TEST-1", entryFor("888", "0.20", nyaa::domain::GpuConfig::Mode::Default)) == 2);
    assert(playlist.addTo("TEST-2", entryFor("888", "0.30", nyaa::domain::GpuConfig::Mode::Nvidia)) == 1);
    assert(stored("VIEW-0").value("entries").toArray().isEmpty());
    assert(stored("TEST-1").value("entries").toArray().size() == 2);

    // D3: each entry keeps its WallpaperConfig and is rendered from it,
    // per entry GPU included
    QJsonArray one = stored("TEST-1").value("entries").toArray();
    assert(argvHas(one[0].toObject(), "fov=0.90"));
    assert(argvHas(one[0].toObject(), "TEST-1"));          // --screen-root TEST-1
    assert(one[0].toObject().value("env").toArray().contains(QJsonValue("DRI_PRIME=2")));
    assert(one[1].toObject().value("env").toArray().isEmpty());
    assert(!one[0].toObject().value("config").toObject().isEmpty());
    const QJsonArray two = stored("TEST-2").value("entries").toArray();
    assert(argvHas(two[0].toObject(), "TEST-2"));
    assert(two[0].toObject().value("env").toArray().contains(QJsonValue("__NV_PRIME_RENDER_OFFLOAD=1")));

    // ---- D1: editing never wipes the other entries -----------------------
    playlist.load("TEST-1");
    spin(300);
    assert(playlist.entries().size() == 2);
    playlist.move(0, 1);
    spin(200);
    one = stored("TEST-1").value("entries").toArray();
    assert(one.size() == 2);
    assert(one[0].toObject().value("wallpaperId").toString() == "888");
    for (const QJsonValue &e : one)
        assert(!e.toObject().value("argv").toArray().isEmpty());   // was [] before
    assert(argvHas(one[1].toObject(), "fov=0.90"));

    playlist.removeAt(0);
    spin(200);
    one = stored("TEST-1").value("entries").toArray();
    assert(one.size() == 1 && one[0].toObject().value("wallpaperId").toString() == "777");
    assert(argvHas(one[0].toObject(), "fov=0.90"));
    assert(stored("TEST-1").value("currentIndex").toInt() == 0);    // D7: clamped

    // ---- D4/D5: mode, interval, loop are saved, with Java's names --------
    playlist.setMode("RANDOM");
    playlist.setInterval(30);
    playlist.setLoop(false);
    spin(200);
    const QJsonObject doc = stored("TEST-1");
    assert(doc.value("mode").toString() == "random");
    assert(doc.value("intervalSec").toInt() == 30);
    assert(!doc.value("loop").toBool());
    PlaylistBridge other(library, config);
    other.load("TEST-1");
    spin(300);
    assert(other.mode() == "RANDOM" && other.interval() == 30 && !other.loop());
    playlist.setInterval(2);                          // the spinner's floor is 5
    spin(200);
    assert(stored("TEST-1").value("intervalSec").toInt() == 5);

    // ---- D8/D9: monitors with playlists are listed; delete removes one ---
    assert(listed().contains("TEST-1") && listed().contains("TEST-2"));
    spin(200);
    assert(playlist.monitors().contains("TEST-2") && playlist.monitors().contains("CONNECTED-1"));
    playlist.load("TEST-2");
    spin(200);
    playlist.deletePlaylist();
    spin(200);
    assert(!listed().contains("TEST-2"));
    assert(listed().contains("TEST-1"));
    assert(stored("TEST-2").value("entries").toArray().isEmpty());

    std::puts("playlist tests passed");
    return 0;
}
