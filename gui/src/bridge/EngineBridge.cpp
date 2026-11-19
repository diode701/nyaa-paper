#include "EngineBridge.hpp"

#include "domain/Renderer.h"
#include "domain/Types.hpp"
#include "domain/options/GlobalOptions.h"
#include "domain/settings/SettingFactory.h"
#include "engine/DbusEngine.hpp"
#include "engine/MockEngine.hpp"
#include "io/ProjectJson.h"
#include "util/VideoDuration.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include <QGuiApplication>
#include <QScreen>

EngineRequest EngineBridge::buildRequest(const QString &monitor, const QString &wallpaperId,
                                         const QString &basePath)
{
    using namespace nyaa::domain;

    Wallpaper wallpaper;
    wallpaper.id = wallpaperId.toStdString();
    wallpaper.basePath = basePath.toStdString();

    // The wallpaper's own properties, at their project.json defaults.
    settings::SettingList list;
    const std::string project = wallpaper.basePath + "/project.json";
    for (const auto &desc : nyaa::io::parseProjectFile(project))
        list.add(settings::makeSetting(desc));

    // Engine-wide defaults from config.json.
    options::GlobalOptions globals;
    const auto &engine = m_config.engine;
    globals.restore({
        {"volume", engine.muted ? "silent|" + std::to_string(engine.volume)
                                : std::to_string(engine.volume)},
        {"fps", (engine.fpsLimited ? "true|" : "false|") + std::to_string(engine.fps)},
        {"scaling", engine.scaling},
        {"clamp", engine.clamp.empty() ? "false|border" : "true|" + engine.clamp},
    });
    for (const auto &flag : engine.flags)
        globals.setFlag(flag, true);

    EngineOptions options;
    options.workshopRoot = m_config.paths.workshop;
    options.assetsDir = m_config.paths.assets;
    options.layer = engine.layer;

    const nyaa::domain::EngineCommand rendered = Renderer::render(monitor.toStdString(), wallpaper, list, globals,
                                                    GpuConfig{}, Screenshot{}, options);

    EngineRequest command;
    command.wallpaperId = wallpaperId;
    for (const auto &arg : rendered.argv)
        command.argv << QString::fromStdString(arg);
    for (const auto &entry : rendered.env)
        command.env << QString::fromStdString(entry);
    return command;
}

void EngineBridge::applyWallpaper(const QString &monitor, const QString &wallpaperId,
                                  const QString &basePath)
{
    m_engine->apply(monitor, buildRequest(monitor, wallpaperId, basePath));
}

QVariantMap EngineBridge::playlistEntry(const QString &monitor, const QString &wallpaperId,
                                        const QString &basePath)
{
    const EngineRequest command = buildRequest(monitor, wallpaperId, basePath);

    // Video wallpapers should play to the end before the playlist advances.
    double duration = 0.0;
    QFile project(basePath + QStringLiteral("/project.json"));
    if (project.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(project.readAll()).object();
        if (root.value(QStringLiteral("type")).toString() == QStringLiteral("video")) {
            const QString file = root.value(QStringLiteral("file")).toString();
            if (!file.isEmpty())
                duration = nyaa::util::videoDuration(
                    (basePath + QLatin1Char('/') + file).toStdString());
        }
    }

    QVariantMap entry;
    entry[QStringLiteral("wallpaperId")] = wallpaperId;
    entry[QStringLiteral("argv")] = command.argv;
    entry[QStringLiteral("env")] = command.env;
    entry[QStringLiteral("durationSec")] = duration > 0.0 ? duration : 0.0;
    return entry;
}

EngineBridge::EngineBridge(const nyaa::io::AppConfig &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    for (const QScreen *screen : QGuiApplication::screens()) {
        const QString name = screen->name();
        if (!name.isEmpty())
            m_monitors << name;
    }
    if (m_monitors.isEmpty())
        m_monitors = {QStringLiteral("eDP-1"), QStringLiteral("HDMI-1")};

    m_mock = qEnvironmentVariableIsSet("NYAA_MOCK");
    if (m_mock)
        m_engine = std::make_unique<MockEngine>();
    else
        m_engine = std::make_unique<DbusEngine>();

    connect(m_engine.get(), &IEngine::infoChanged, this, &EngineBridge::infoChanged);
    connect(m_engine.get(), &IEngine::engineExited, this, &EngineBridge::engineExited);
    connect(m_engine.get(), &IEngine::daemonError, this, &EngineBridge::error);
    connect(m_engine.get(), &IEngine::runningChanged, this, [this](const QMap<QString, int> &running) {
        m_running.clear();
        for (auto it = running.cbegin(); it != running.cend(); ++it)
            m_running.insert(it.key(), it.value());
        emit runningChanged();
    });
}
