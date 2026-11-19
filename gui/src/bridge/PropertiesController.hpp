#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include <vector>

#include "domain/WallpaperConfig.h"
#include "domain/settings/Setting.h"
#include "engine/IEngine.hpp"
#include "io/WallpaperStore.h"

class ConfigBridge;
class EngineBridge;
class GlobalOptionsModel;
class PlaylistBridge;
class PropertyModel;
class WallpaperModel;

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
    // Capture & theme: on a successful apply, recolour the window from it
    Q_PROPERTY(bool themeFromWallpaper READ themeFromWallpaper WRITE setThemeFromWallpaper NOTIFY screenshotChanged)
    Q_PROPERTY(QString themeVia READ themeVia WRITE setThemeVia NOTIFY screenshotChanged)   // builtin | pywal
    Q_PROPERTY(bool pywalAvailable READ pywalAvailable CONSTANT)
    /// Bumped after every successful apply, so views showing what runs refresh
    Q_PROPERTY(int revision READ revision NOTIFY revisionChanged)

public:
    PropertiesController(PropertyModel &properties, GlobalOptionsModel &globals, EngineBridge &engine,
                         PlaylistBridge &playlist, ConfigBridge &config,
                         const WallpaperModel &library, QObject *parent = nullptr);

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
    bool themeFromWallpaper() const { return m_themeFromWallpaper; }
    void setThemeFromWallpaper(bool on) { m_themeFromWallpaper = on; emit screenshotChanged(); }
    QString themeVia() const { return m_themeVia; }
    void setThemeVia(const QString &via) { m_themeVia = via; emit screenshotChanged(); }
    bool pywalAvailable() const { return m_pywal; }
    int revision() const { return m_revision; }
    /// Preview of what a monitor last ran (its current WallpaperConfig), or "".
    Q_INVOKABLE QString runningPreview(const QString &monitor) const;

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
    void revisionChanged();

private:
    void onApplyFinished(const QString &monitor, bool ok, const QString &message);
    /// Java's allSettings: the global options, then the wallpaper's own
    /// properties, as one list of Settings.
    std::vector<nyaa::domain::settings::Setting *> allSettings();
    std::vector<const nyaa::domain::settings::Setting *> allSettings() const;

    PropertyModel &m_properties;
    GlobalOptionsModel &m_globals;
    EngineBridge &m_engine;
    PlaylistBridge &m_playlist;
    ConfigBridge &m_config;   // read for paths; written by "Theme from this wallpaper"
    const WallpaperModel &m_library;
    nyaa::io::WallpaperStore m_store;

    /// The open wallpaper (Java: the view's Wppr). From the library when it is
    /// there, so type and file need no second read of project.json.
    nyaa::domain::Wallpaper m_wallpaper;
    QString m_wallpaperId;
    QString m_basePath;
    nyaa::domain::GpuConfig m_gpu;
    bool m_nvidia = false;
    bool m_themeFromWallpaper = false;
    QString m_themeVia = QStringLiteral("builtin");
    bool m_pywal = false;
    int m_revision = 0;
    bool m_screenshotEnabled = false;
    QString m_screenshotPath;
    int m_screenshotDelay = 5;

    /// Configs waiting for the daemon's answer, saved only if it succeeds.
    QHash<QString, nyaa::domain::WallpaperConfig> m_pending;
};
