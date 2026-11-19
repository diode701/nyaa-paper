#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

#include "io/AppConfig.h"
#include "io/ConfigStore.h"

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

    Q_PROPERTY(QString layer READ layer WRITE setLayer NOTIFY changed)
    Q_PROPERTY(bool persistOnStart READ persistOnStart WRITE setPersistOnStart NOTIFY changed)
    Q_PROPERTY(bool autoStartOnBoot READ autoStartOnBoot WRITE setAutoStartOnBoot NOTIFY changed)
    /// Comma separated, as typed in Settings
    Q_PROPERTY(QString customMonitors READ customMonitors WRITE setCustomMonitors NOTIFY changed)
    Q_PROPERTY(QString palette READ palette WRITE setPalette NOTIFY changed)
    Q_PROPERTY(QString themeSource READ themeSource WRITE setThemeSource NOTIFY changed)
    Q_PROPERTY(QString hoverStyle READ hoverStyle WRITE setHoverStyle NOTIFY changed)
    Q_PROPERTY(QString selectionStyle READ selectionStyle WRITE setSelectionStyle NOTIFY changed)
    Q_PROPERTY(bool backdrop READ backdrop WRITE setBackdrop NOTIFY changed)
    Q_PROPERTY(bool tint READ tint WRITE setTint NOTIFY changed)
    Q_PROPERTY(double uiScale READ uiScale WRITE setUiScale NOTIFY changed)
    Q_PROPERTY(QString renderer READ renderer WRITE setRenderer NOTIFY changed)
    Q_PROPERTY(bool titleScreen READ titleScreen WRITE setTitleScreen NOTIFY changed)

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
    QString layer() const;
    bool persistOnStart() const { return m_config.startup.persistOnStart; }
    bool autoStartOnBoot() const { return m_config.startup.autoStartOnBoot; }
    QString customMonitors() const;
    void setPersistOnStart(bool on);
    void setAutoStartOnBoot(bool on);
    void setCustomMonitors(const QString &commaList);

    QString palette() const { return QString::fromStdString(m_config.ui.palette); }
    QString themeSource() const { return QString::fromStdString(m_config.ui.themeSource); }
    QString hoverStyle() const { return QString::fromStdString(m_config.ui.hoverStyle); }
    QString selectionStyle() const { return QString::fromStdString(m_config.ui.selectionStyle); }
    bool backdrop() const { return m_config.ui.backdrop; }
    bool tint() const { return m_config.ui.tint; }
    double uiScale() const { return m_config.ui.scale; }
    QString renderer() const { return QString::fromStdString(m_config.ui.renderer); }
    bool titleScreen() const { return m_config.ui.titleScreen; }
    void setPalette(const QString &v) { m_config.ui.palette = v.toStdString(); emit changed(); }
    void setThemeSource(const QString &v) { m_config.ui.themeSource = v.toStdString(); emit changed(); }
    void setHoverStyle(const QString &v) { m_config.ui.hoverStyle = v.toStdString(); emit changed(); }
    void setSelectionStyle(const QString &v) { m_config.ui.selectionStyle = v.toStdString(); emit changed(); }
    void setBackdrop(bool v) { m_config.ui.backdrop = v; emit changed(); }
    void setTint(bool v) { m_config.ui.tint = v; emit changed(); }
    void setUiScale(double v) { m_config.ui.scale = qBound(0.75, v, 2.0); emit changed(); }
    void setRenderer(const QString &v) { m_config.ui.renderer = v.toStdString(); emit changed(); }
    void setTitleScreen(bool v) { m_config.ui.titleScreen = v; emit changed(); }

    void setWorkshopPath(const QString &value);
    void setEnginePath(const QString &value);
    void setAssetsPath(const QString &value);
    void setScreenshotsPath(const QString &value);
    void setPywalPath(const QString &value);
    void setAnimation(const QString &value);
    void setPageSlideMs(int value);
    void setLayer(const QString &value);

    Q_INVOKABLE bool save();
    Q_INVOKABLE void reload();
    Q_INVOKABLE QString filePath() const;
    /// Fire-and-forget: asks the daemon to re-read config.json and rescan.
    Q_INVOKABLE void rescanLibrary();

Q_SIGNALS:
    void changed();
    void saved(bool ok);
    void problem(const QString &message);

private:
    nyaa::io::ConfigStore m_store;
    nyaa::io::AppConfig m_config;
};
