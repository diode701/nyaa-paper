#include "PropertiesController.hpp"

#include <set>

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>

#include "bridge/ConfigBridge.hpp"
#include "bridge/EngineBridge.hpp"
#include "bridge/PlaylistBridge.hpp"
#include "domain/Renderer.h"
#include "models/GlobalOptionsModel.hpp"
#include "models/PropertyModel.hpp"
#include "util/VideoDuration.h"

using nyaa::domain::GpuConfig;
using nyaa::domain::WallpaperConfig;

PropertiesController::PropertiesController(PropertyModel &properties, GlobalOptionsModel &globals,
                                           EngineBridge &engine, PlaylistBridge &playlist,
                                           const ConfigBridge &config, QObject *parent)
    : QObject(parent)
    , m_properties(properties)
    , m_globals(globals)
    , m_engine(engine)
    , m_playlist(playlist)
    , m_config(config)
{
    connect(&m_engine, &EngineBridge::applyFinished, this, &PropertiesController::onApplyFinished);
}

// ---- GPU / screenshot state ------------------------------------------------

QString PropertiesController::gpuMode() const
{
    return QString::fromStdString(nyaa::domain::gpuModeName(m_gpu.mode));
}

void PropertiesController::setGpuMode(const QString &mode)
{
    m_gpu.mode = nyaa::domain::gpuModeFromName(mode.toStdString());
    emit gpuChanged();
}

void PropertiesController::setDriPrimeNumber(int number)
{
    m_gpu.driPrimeNumber = number;
    emit gpuChanged();
}

void PropertiesController::setGlThreaded(bool on)
{
    m_gpu.glThreaded = on;
    emit gpuChanged();
}

void PropertiesController::setScreenshotEnabled(bool on)
{
    m_screenshotEnabled = on;
    emit screenshotChanged();
}

void PropertiesController::setScreenshotPath(const QString &path)
{
    m_screenshotPath = path;
    emit screenshotChanged();
}

void PropertiesController::setScreenshotDelay(int frames)
{
    m_screenshotDelay = frames;
    emit screenshotChanged();
}

// ---- open ------------------------------------------------------------------

int PropertiesController::open(const QString &wallpaperId, const QString &basePath)
{
    // Java made a new WallpaperPropertiesView per wallpaper, so every control
    // started from its default. Same here.
    m_wallpaperId = wallpaperId;
    m_basePath = basePath;
    m_properties.loadWallpaper(basePath);
    m_globals.reset();

    m_gpu = GpuConfig{};   // Default (auto), GPU index 1, GL threading off
    emit gpuChanged();

    m_screenshotEnabled = false;
    m_screenshotPath = m_config.screenshotsPath();   // Settings.getScreenshotPath()
    m_screenshotDelay = 5;
    emit screenshotChanged();

    return m_properties.rowCount();
}

// ---- render / capture ------------------------------------------------------

EngineRequest PropertiesController::render(const QString &monitor, bool withScreenshot) const
{
    nyaa::domain::Wallpaper wallpaper;
    wallpaper.id = m_wallpaperId.toStdString();
    wallpaper.basePath = m_basePath.toStdString();

    nyaa::domain::Screenshot screenshot;
    if (withScreenshot && m_screenshotEnabled) {
        screenshot.directory = m_screenshotPath.trimmed().toStdString();
        screenshot.delay = m_screenshotDelay;
    }

    nyaa::domain::EngineOptions options;
    options.workshopRoot = m_config.config().paths.workshop;
    options.assetsDir = m_config.config().paths.assets;
    options.layer = m_config.config().engine.layer;

    const nyaa::domain::EngineCommand command = nyaa::domain::Renderer::render(
        monitor.toStdString(), wallpaper, m_properties.settings(), m_globals.options(), m_gpu,
        screenshot, options);

    EngineRequest request;
    request.wallpaperId = m_wallpaperId;
    for (const auto &arg : command.argv)
        request.argv << QString::fromStdString(arg);
    for (const auto &entry : command.env)
        request.env << QString::fromStdString(entry);
    return request;
}

WallpaperConfig PropertiesController::capture(const QString &monitor) const
{
    WallpaperConfig config;
    config.wallpaperId = m_wallpaperId.toStdString();
    config.monitor = monitor.toStdString();
    // getProjectJsonTimestamp()
    const QFileInfo project(m_basePath + QStringLiteral("/project.json"));
    config.projectJsonModified = project.exists() ? project.lastModified().toMSecsSinceEpoch() : 0;
    config.gpu = m_gpu;
    // Java: one loop over allSettings (custom + global)
    config.settings = m_properties.settings().serialize();
    for (const auto &[name, value] : m_globals.options().serialize())
        config.settings[name] = value;
    return config;
}

// ---- actions ---------------------------------------------------------------

void PropertiesController::apply(const QString &monitorName)
{
    const QString monitor = monitorName.trimmed();
    if (monitor.isEmpty()) {
        emit status(tr("Please select a monitor."));
        return;
    }

    if (m_playlist.pauseIfPlaying(monitor))
        emit status(tr("Playlist paused — manual apply on %1").arg(monitor));

    if (m_screenshotEnabled && m_screenshotPath.trimmed().isEmpty()) {
        emit status(tr("Please enter a screenshot output path."));
        return;
    }

    emit status(tr("Applying..."));
    m_pending.insert(monitor, capture(monitor));
    m_engine.apply(monitor, render(monitor, true));
}

void PropertiesController::onApplyFinished(const QString &monitor, bool ok, const QString &message)
{
    const auto it = m_pending.find(monitor);
    if (it == m_pending.end())
        return;   // not ours (a playlist step, for example)
    const WallpaperConfig config = it.value();
    m_pending.erase(it);

    if (!ok) {
        emit status(tr("Error: %1").arg(message));
        return;
    }
    // Java saves the config only after the engine started.
    m_store.save(config);
    emit status(tr("Applied to %1").arg(monitor));
}

void PropertiesController::loadLast(const QString &monitorName)
{
    const QString monitor = monitorName.trimmed();
    if (monitor.isEmpty()) {
        emit status(tr("Please select a monitor first."));
        return;
    }

    // This wallpaper's own config on this monitor, even if another wallpaper
    // was applied there since (Java lost it: one file per monitor).
    const WallpaperConfig saved = m_store.load(monitor.toStdString(), m_wallpaperId.toStdString());
    if (saved.empty() || saved.settings.empty()) {
        emit status(tr("No saved settings for this wallpaper on %1").arg(monitor));
        return;
    }

    // Java counts every setting (custom or global) whose name is in the file.
    int applied = 0;
    for (const auto &setting : m_properties.settings().all())
        applied += saved.settings.count(setting->getName()) ? 1 : 0;
    for (const auto *option : m_globals.options().all())
        applied += saved.settings.count(option->getName()) ? 1 : 0;

    if (applied == 0) {
        emit status(tr("Saved config found but no matching settings."));
        return;
    }

    m_properties.settings().restore(saved.settings);
    m_properties.refreshAll();
    m_globals.restore(saved.settings);
    // Java's loadSavedSettings leaves the GPU radios alone; so does this.
    emit status(tr("Loaded %1 saved settings for %2").arg(applied).arg(monitor));
}

void PropertiesController::stop(const QString &monitorName)
{
    const QString monitor = monitorName.trimmed();
    if (monitor.isEmpty()) {
        emit status(tr("Please select a monitor."));
        return;
    }
    m_engine.stop(monitor);
    emit status(tr("Stopped on %1").arg(monitor));
}

QVariantMap PropertiesController::playlistEntry(const QString &monitorName)
{
    const QString monitor = monitorName.trimmed();
    // Java's playlist applies with no screenshot (apply(..., null, 0)).
    const EngineRequest request = render(monitor, false);

    // Video wallpapers play to the end before the playlist moves on.
    double duration = 0.0;
    QFile project(m_basePath + QStringLiteral("/project.json"));
    if (project.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(project.readAll()).object();
        if (root.value(QStringLiteral("type")).toString().compare(QStringLiteral("video"), Qt::CaseInsensitive) == 0) {
            const QString file = root.value(QStringLiteral("file")).toString();
            if (!file.isEmpty())
                duration = nyaa::util::videoDuration((m_basePath + QLatin1Char('/') + file).toStdString());
        }
    }

    QVariantMap entry;
    entry[QStringLiteral("wallpaperId")] = m_wallpaperId;
    entry[QStringLiteral("argv")] = request.argv;
    entry[QStringLiteral("env")] = request.env;
    entry[QStringLiteral("durationSec")] = duration > 0.0 ? duration : 0.0;
    return entry;
}

void PropertiesController::forgetUnsubscribed(const QStringList &activeIds)
{
    std::set<std::string> ids;
    for (const QString &id : activeIds)
        ids.insert(id.toStdString());
    m_store.cleanupStale(ids);
}
