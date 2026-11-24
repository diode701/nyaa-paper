#include "PropertiesController.hpp"

#include <set>

#include <QProcess>
#include <QStandardPaths>
#include <QUrl>

#include <QDateTime>
#include <QFileInfo>

#include "bridge/ConfigBridge.hpp"
#include "bridge/EngineBridge.hpp"
#include "bridge/PlaylistBridge.hpp"
#include "domain/Renderer.h"
#include "io/WallpaperConfigJson.h"
#include "models/GlobalOptionsModel.hpp"
#include "models/PropertyModel.hpp"
#include "models/WallpaperModel.hpp"
#include "util/Gpu.h"
#include "util/VideoDuration.h"

using nyaa::domain::GpuConfig;
using nyaa::domain::WallpaperConfig;

PropertiesController::PropertiesController(PropertyModel &properties, GlobalOptionsModel &globals,
                                           EngineBridge &engine, PlaylistBridge &playlist,
                                           ConfigBridge &config, const WallpaperModel &library,
                                           QObject *parent)
    : QObject(parent)
    , m_properties(properties)
    , m_globals(globals)
    , m_engine(engine)
    , m_playlist(playlist)
    , m_config(config)
    , m_library(library)
{
    connect(&m_engine, &EngineBridge::applyFinished, this, &PropertiesController::onApplyFinished);
    m_nvidia = nyaa::util::detectNvidia();   // once, like the Java launcher
    m_pywal = !QStandardPaths::findExecutable(QStringLiteral("wal")).isEmpty();
}

QString PropertiesController::runningPreview(const QString &monitor) const
{
    const nyaa::domain::WallpaperConfig current = m_store.current(monitor.toStdString());
    if (current.empty())
        return {};
    if (const nyaa::domain::Wallpaper *w = m_library.find(QString::fromStdString(current.wallpaperId)))
        return QString::fromStdString(w->preview);
    return {};
}

QString PropertiesController::runningId(const QString &monitor) const
{
    return QString::fromStdString(m_store.current(monitor.toStdString()).wallpaperId);
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
    if (const nyaa::domain::Wallpaper *known = m_library.find(wallpaperId))
        m_wallpaper = *known;
    else
        m_wallpaper = WallpaperModel::make(wallpaperId, wallpaperId, {}, {}, basePath);
    m_properties.loadWallpaper(basePath);   // the one read of project.json
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

std::vector<nyaa::domain::settings::Setting *> PropertiesController::allSettings()
{
    std::vector<nyaa::domain::settings::Setting *> all;
    for (auto *option : m_globals.options().all())
        all.push_back(option);
    for (const auto &setting : m_properties.settings().all())
        all.push_back(setting.get());
    return all;
}

std::vector<const nyaa::domain::settings::Setting *> PropertiesController::allSettings() const
{
    std::vector<const nyaa::domain::settings::Setting *> all;
    for (const auto *option : m_globals.options().all())
        all.push_back(option);
    for (const auto &setting : m_properties.settings().all())
        all.push_back(setting.get());
    return all;
}

EngineRequest PropertiesController::render(const QString &monitor, bool withScreenshot) const
{
    const nyaa::domain::Wallpaper &wallpaper = m_wallpaper;

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
        monitor.toStdString(), wallpaper, allSettings(), m_gpu, screenshot, options);

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
    // captureConfig: one loop over allSettings; labels and group headers
    // carry no value (Java's getValueString() returned null for them)
    using nyaa::domain::settings::Kind;
    for (const auto *setting : allSettings()) {
        if (setting->getKind() == Kind::Text || setting->getKind() == Kind::Group)
            continue;
        config.settings[setting->getName()] = setting->getValueString();
    }
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
    ++m_revision;
    emit revisionChanged();
    emit status(tr("Applied to %1").arg(monitor));

    // Capture & theme: take the window's colours from what was just applied
    if (m_themeFromWallpaper && !m_wallpaper.preview.empty()) {
        ConfigBridge &config = m_config;
        const QString preview = QString::fromStdString(m_wallpaper.preview);
        if (m_themeVia == QStringLiteral("pywal") && m_pywal) {
            const QUrl url(preview);
            // -n: leave the desktop wallpaper alone, -q: quiet, -s -t: no terminal changes
            QProcess::startDetached(QStringLiteral("wal"),
                                    {QStringLiteral("-n"), QStringLiteral("-q"), QStringLiteral("-s"), QStringLiteral("-t"),
                                     QStringLiteral("-i"), url.isLocalFile() ? url.toLocalFile() : preview});
            config.setThemeSource(QStringLiteral("pywal"));
        } else {
            config.setThemeWallpaper(preview);
            config.setThemeSource(QStringLiteral("wallpaper"));
        }
        config.save();
    }
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

    // loadSavedSettings: one loop over allSettings, counting what matched
    int applied = 0;
    for (auto *setting : allSettings()) {
        const auto it = saved.settings.find(setting->getName());
        if (it == saved.settings.end())
            continue;
        setting->setValueFromString(it->second);
        ++applied;
    }

    if (applied == 0) {
        emit status(tr("Saved config found but no matching settings."));
        return;
    }
    m_properties.refreshAll();
    m_globals.refresh();   // rebuildUI()
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

    // PlaylistRunner.getDuration: a video plays to its end before the
    // playlist moves on. Type and file come from the library, not a re-read.
    double duration = 0.0;
    if (m_wallpaper.isVideo() && !m_wallpaper.file.empty() && m_wallpaper.file != "unknown file")
        duration = nyaa::util::videoDuration(m_wallpaper.basePath + "/" + m_wallpaper.file);

    QVariantMap entry;
    // Java: the entry IS the captured WallpaperConfig. The playlist bridge
    // re-renders argv/env from it whenever it saves the playlist.
    entry[QStringLiteral("config")] = nyaa::io::toJson(capture(monitor)).toVariantMap();
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
