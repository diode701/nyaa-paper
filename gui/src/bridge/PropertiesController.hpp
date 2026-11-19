#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "domain/WallpaperConfig.h"
#include "engine/IEngine.hpp"
#include "io/WallpaperStore.h"

class ConfigBridge;
class EngineBridge;
class GlobalOptionsModel;
class PlaylistBridge;
class PropertyModel;

/// The logic half of Java's WallpaperPropertiesView: everything that view did
/// besides drawing. One wallpaper is open at a time; its custom settings live
/// in PropertyModel, its global options in GlobalOptionsModel, and the GPU and
/// screenshot choices here. All of it belongs to this apply on this monitor:
/// nothing is global, so every monitor can run its own wallpaper on its own
/// GPU with its own options.
class PropertiesController : public QObject
{
    Q_OBJECT
    // GPU section (Java: the radio group, DRI spinner and GL checkbox)
    Q_PROPERTY(QString gpuMode READ gpuMode WRITE setGpuMode NOTIFY gpuChanged)
    Q_PROPERTY(int driPrimeNumber READ driPrimeNumber WRITE setDriPrimeNumber NOTIFY gpuChanged)
    Q_PROPERTY(bool glThreaded READ glThreaded WRITE setGlThreaded NOTIFY gpuChanged)
    /// Java: the NVIDIA radio is shown only when NVIDIA is detected
    Q_PROPERTY(bool nvidiaAvailable READ nvidiaAvailable CONSTANT)
    // Screenshot section
    Q_PROPERTY(bool screenshotEnabled READ screenshotEnabled WRITE setScreenshotEnabled NOTIFY screenshotChanged)
    Q_PROPERTY(QString screenshotPath READ screenshotPath WRITE setScreenshotPath NOTIFY screenshotChanged)
    Q_PROPERTY(int screenshotDelay READ screenshotDelay WRITE setScreenshotDelay NOTIFY screenshotChanged)

public:
    PropertiesController(PropertyModel &properties, GlobalOptionsModel &globals, EngineBridge &engine,
                         PlaylistBridge &playlist, const ConfigBridge &config, QObject *parent = nullptr);

    QString gpuMode() const;
    int driPrimeNumber() const { return m_gpu.driPrimeNumber; }
    bool glThreaded() const { return m_gpu.glThreaded; }
    bool nvidiaAvailable() const { return m_nvidia; }
    bool screenshotEnabled() const { return m_screenshotEnabled; }
    QString screenshotPath() const { return m_screenshotPath; }
    int screenshotDelay() const { return m_screenshotDelay; }

    void setGpuMode(const QString &mode);
    void setDriPrimeNumber(int number);
    void setGlThreaded(bool on);
    void setScreenshotEnabled(bool on);
    void setScreenshotPath(const QString &path);
    void setScreenshotDelay(int frames);

    /// A new properties view: parse the wallpaper, fresh global options, GPU
    /// back to default. Returns the number of custom properties.
    Q_INVOKABLE int open(const QString &wallpaperId, const QString &basePath);
    /// applyToMonitor
    Q_INVOKABLE void apply(const QString &monitor);
    /// loadSavedSettings
    Q_INVOKABLE void loadLast(const QString &monitor);
    /// stopOnMonitor
    Q_INVOKABLE void stop(const QString &monitor);
    /// The playlist entry addToPlaylist stores, rendered from the edited values.
    Q_INVOKABLE QVariantMap playlistEntry(const QString &monitor);

    /// captureConfig: every custom and global value plus the GPU.
    nyaa::domain::WallpaperConfig capture(const QString &monitor) const;
    /// The exact command apply() would send, without sending it.
    EngineRequest render(const QString &monitor, bool withScreenshot) const;
    /// Builder.build → cleanupStale: forget monitors whose wallpaper is gone.
    void forgetUnsubscribed(const QStringList &activeIds);

Q_SIGNALS:
    void gpuChanged();
    void screenshotChanged();
    /// The status line under the properties (Java: statusLabel.setText).
    void status(const QString &message);

private:
    void onApplyFinished(const QString &monitor, bool ok, const QString &message);

    PropertyModel &m_properties;
    GlobalOptionsModel &m_globals;
    EngineBridge &m_engine;
    PlaylistBridge &m_playlist;
    const ConfigBridge &m_config;
    nyaa::io::WallpaperStore m_store;

    QString m_wallpaperId;
    QString m_basePath;
    nyaa::domain::GpuConfig m_gpu;
    bool m_nvidia = false;
    bool m_screenshotEnabled = false;
    QString m_screenshotPath;
    int m_screenshotDelay = 5;

    /// Configs waiting for the daemon's answer, saved only if it succeeds.
    QHash<QString, nyaa::domain::WallpaperConfig> m_pending;
};
