#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "io/AppConfig.h"
#include "io/ConfigStore.h"
#include "io/WallpaperStore.h"

/// The QML handle on config.json. One instance, owned by main; the file is the
/// single source of truth for both this process and the daemon.
class ConfigBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString workshopPath READ workshopPath WRITE setWorkshopPath NOTIFY changed)
    Q_PROPERTY(QString enginePath READ enginePath WRITE setEnginePath NOTIFY changed)
    Q_PROPERTY(QString assetsPath READ assetsPath WRITE setAssetsPath NOTIFY changed)
    Q_PROPERTY(QString screenshotsPath READ screenshotsPath WRITE setScreenshotsPath NOTIFY changed)
    Q_PROPERTY(QString pywalPath READ pywalPath WRITE setPywalPath NOTIFY changed)

    Q_PROPERTY(QString animation READ animation WRITE setAnimation NOTIFY changed)
    Q_PROPERTY(int pageSlideMs READ pageSlideMs WRITE setPageSlideMs NOTIFY changed)

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY changed)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY changed)
    Q_PROPERTY(bool fpsLimited READ fpsLimited WRITE setFpsLimited NOTIFY changed)
    Q_PROPERTY(int fps READ fps WRITE setFps NOTIFY changed)
    Q_PROPERTY(QString scaling READ scaling WRITE setScaling NOTIFY changed)
    Q_PROPERTY(QString clampMode READ clampMode WRITE setClampMode NOTIFY changed)
    Q_PROPERTY(QString layer READ layer WRITE setLayer NOTIFY changed)

public:
    explicit ConfigBridge(QObject *parent = nullptr);

    const nyaa::io::AppConfig &config() const { return m_config; }

    QString workshopPath() const;
    QString enginePath() const;
    QString assetsPath() const;
    QString screenshotsPath() const;
    QString pywalPath() const;
    QString animation() const;
    int pageSlideMs() const;
    int volume() const;
    bool muted() const;
    bool fpsLimited() const;
    int fps() const;
    QString scaling() const;
    QString clampMode() const;
    QString layer() const;

    void setWorkshopPath(const QString &value);
    void setEnginePath(const QString &value);
    void setAssetsPath(const QString &value);
    void setScreenshotsPath(const QString &value);
    void setPywalPath(const QString &value);
    void setAnimation(const QString &value);
    void setPageSlideMs(int value);
    void setVolume(int value);
    void setMuted(bool value);
    void setFpsLimited(bool value);
    void setFps(int value);
    void setScaling(const QString &value);
    void setClampMode(const QString &value);
    void setLayer(const QString &value);

    Q_INVOKABLE bool save();
    Q_INVOKABLE void reload();
    Q_INVOKABLE QString filePath() const;
    /// Fire-and-forget: asks the daemon to re-read config.json and rescan.
    Q_INVOKABLE void rescanLibrary();
    /// Remembers the applied values for a monitor, and reads them back.
    Q_INVOKABLE bool saveApplied(const QString &monitor, const QString &wallpaperId,
                                 const QVariantMap &values);
    Q_INVOKABLE QVariantMap loadApplied(const QString &monitor);

Q_SIGNALS:
    void changed();
    void saved(bool ok);

private:
    nyaa::io::ConfigStore m_store;
    nyaa::io::WallpaperStore m_wallpapers;
    nyaa::io::AppConfig m_config;
};
