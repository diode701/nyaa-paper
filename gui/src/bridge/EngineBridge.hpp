#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QVariantList>
#include <memory>

#include "engine/IEngine.hpp"
#include "io/AppConfig.h"

/// The QML-facing engine API. Everything QML may call about playback lives
/// here; QML never sees D-Bus, never sees argv, never sees a PID it can misuse.
class EngineBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version NOTIFY infoChanged)
    Q_PROPERTY(QString enginePath READ enginePath NOTIFY infoChanged)
    Q_PROPERTY(QVariantMap running READ running NOTIFY runningChanged)
    Q_PROPERTY(QStringList monitors READ monitors CONSTANT)
    Q_PROPERTY(bool mock READ mock CONSTANT)

public:
    explicit EngineBridge(const nyaa::io::AppConfig &config, QObject *parent = nullptr);

    QString version() const { return m_engine->version(); }
    QString enginePath() const { return m_engine->enginePath(); }
    QVariantMap running() const { return m_running; }
    QStringList monitors() const { return m_monitors; }
    bool mock() const { return m_mock; }

    Q_INVOKABLE void refreshRunning() { m_engine->refreshRunning(); }
    Q_INVOKABLE QStringList monitorList() const { return m_monitors; }
    Q_INVOKABLE void stop(const QString &monitor) { m_engine->stop(monitor); }
    Q_INVOKABLE void stopAll() { m_engine->stopAll(); }

    /// Apply by id with the bare minimum argv. The domain Renderer will replace
    /// this once the Setting hierarchy lands; the daemon does not care either way.
    /// Builds the real argv through domain::Renderer: parses the wallpaper's
    /// project.json, applies the global defaults and the configured paths.
    Q_INVOKABLE void applyWallpaper(const QString &monitor, const QString &wallpaperId,
                                    const QString &basePath);

    /// One playlist entry for the daemon: the rendered argv, env and the display
    /// duration (video length when it is a video, 0 otherwise).
    Q_INVOKABLE QVariantMap playlistEntry(const QString &monitor, const QString &wallpaperId,
                                          const QString &basePath);

Q_SIGNALS:
    void infoChanged();
    void runningChanged();
    void engineExited(const QString &monitor, int code);
    void error(const QString &message);

private:
    EngineRequest buildRequest(const QString &monitor, const QString &wallpaperId,
                               const QString &basePath);

    std::unique_ptr<IEngine> m_engine;
    const nyaa::io::AppConfig &m_config;
    QVariantMap m_running;
    QStringList m_monitors;
    bool m_mock = false;
};
