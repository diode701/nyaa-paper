#pragma once

#include <QMap>
#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <memory>

#include "engine/IEngine.hpp"

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
    explicit EngineBridge(QObject *parent = nullptr);

    QString version() const { return m_engine->version(); }
    QString enginePath() const { return m_engine->enginePath(); }
    QVariantMap running() const { return m_running; }
    QStringList monitors() const { return m_monitors; }
    bool mock() const { return m_mock; }

    Q_INVOKABLE void refreshRunning() { m_engine->refreshRunning(); }
    Q_INVOKABLE void stop(const QString &monitor) { m_engine->stop(monitor); }
    Q_INVOKABLE void stopAll() { m_engine->stopAll(); }

Q_SIGNALS:
    void infoChanged();
    void runningChanged();
    void engineExited(const QString &monitor, int code);
    void error(const QString &message);

private:
    std::unique_ptr<IEngine> m_engine;
    QVariantMap m_running;
    QStringList m_monitors;
    bool m_mock = false;
};
