#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <memory>

#include "engine/IEngine.hpp"

/// What runs where. Java equivalent: WallpaperManager's public face.
///
/// It does not build commands: PropertiesController renders them from the
/// settings you edited and hands them here. QML never sees argv or D-Bus.
class EngineBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version NOTIFY infoChanged)
    Q_PROPERTY(QString enginePath READ enginePath NOTIFY infoChanged)
    Q_PROPERTY(QVariantMap running READ running NOTIFY runningChanged)
    Q_PROPERTY(QStringList monitors READ monitors NOTIFY monitorsChanged)
    Q_PROPERTY(bool mock READ mock CONSTANT)

public:
    explicit EngineBridge(QObject *parent = nullptr);

    QString version() const { return m_engine->version(); }
    QString enginePath() const { return m_engine->enginePath(); }
    QVariantMap running() const { return m_running; }
    QStringList monitors() const { return m_monitors; }
    bool mock() const { return m_mock; }

    Q_INVOKABLE void refreshRunning() { m_engine->refreshRunning(); }
    Q_INVOKABLE void stop(const QString &monitor) { m_engine->stop(monitor); }
    Q_INVOKABLE void stopAll() { m_engine->stopAll(); }

    /// Detected screens first, then names added by hand (Settings).
    void setCustomMonitors(const QStringList &names);

    /// Run this exact command on the monitor; the answer is applyFinished().
    void apply(const QString &monitor, const EngineRequest &request) { m_engine->apply(monitor, request); }

Q_SIGNALS:
    void infoChanged();
    void runningChanged();
    void monitorsChanged();
    void engineExited(const QString &monitor, int code);
    void applyFinished(const QString &monitor, bool ok, const QString &message);
    void error(const QString &message);

private:
    std::unique_ptr<IEngine> m_engine;
    QVariantMap m_running;
    QStringList m_detected;
    QStringList m_monitors;
    bool m_mock = false;
};
