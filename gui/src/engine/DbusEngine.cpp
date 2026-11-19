#include "DbusEngine.hpp"
#include "engine_proxy.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr auto kService = "org.nyaa.Paper";
constexpr auto kPath = "/org/nyaa/Paper";
}

DbusEngine::DbusEngine(QObject *parent)
    : IEngine(parent)
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit daemonError(tr("cannot reach the session bus"));
        return;
    }

    m_engine = new OrgNyaaPaperEngineInterface(QString::fromLatin1(kService),
                                               QString::fromLatin1(kPath), bus, this);
    connect(m_engine, &OrgNyaaPaperEngineInterface::EngineExited,
            this, &IEngine::engineExited);

    auto *watcher = new QDBusPendingCallWatcher(m_engine->Info(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString, QString> reply = *call;
                if (reply.isError())
                    emit daemonError(reply.error().message());
                else
                    setInfo(reply.argumentAt<0>(), reply.argumentAt<1>());
                call->deleteLater();
            });
}

void DbusEngine::refreshRunning()
{
    if (!m_engine)
        return;

    auto *watcher = new QDBusPendingCallWatcher(m_engine->ListRunning(), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString> reply = *call;
                if (reply.isError()) {
                    emit daemonError(reply.error().message());
                } else {
                    QMap<QString, int> running;
                    const QJsonObject obj = QJsonDocument::fromJson(reply.value().toUtf8()).object();
                    for (auto it = obj.begin(); it != obj.end(); ++it)
                        running.insert(it.key(), it.value().toInt());
                    emit runningChanged(running);
                }
                call->deleteLater();
            });
}

void DbusEngine::apply(const QString &monitor, const EngineRequest &command)
{
    if (!m_engine) {
        emit applyFinished(monitor, false, tr("daemon is not available"));
        return;
    }
    auto *watcher = new QDBusPendingCallWatcher(
        m_engine->Apply(monitor, command.wallpaperId, command.argv, command.env), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, monitor](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<uint> reply = *call;
                if (reply.isError())
                    emit applyFinished(monitor, false, reply.error().message());
                else
                    emit applyFinished(monitor, true, {});
                call->deleteLater();
                refreshRunning();
            });
}

void DbusEngine::stop(const QString &monitor)
{
    if (m_engine)
        m_engine->Stop(monitor);
}

void DbusEngine::stopAll()
{
    // Used on "Quit and stop wallpapers": wait, or the call dies with us.
    if (m_engine)
        m_engine->StopAll().waitForFinished();
}
