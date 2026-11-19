#include "PlaylistBridge.hpp"

#include "playlist_proxy.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
constexpr auto kService = "org.nyaa.Paper";
constexpr auto kPath = "/org/nyaa/Paper";
} // namespace

PlaylistBridge::PlaylistBridge(QObject *parent)
    : QObject(parent)
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        emit failed(tr("cannot reach the session bus"));
        return;
    }

    m_playlist = new OrgNyaaPaperPlaylistInterface(QString::fromLatin1(kService),
                                                   QString::fromLatin1(kPath), bus, this);
    connect(m_playlist, &OrgNyaaPaperPlaylistInterface::StateChanged, this,
            [this](const QString &monitor, const QString &state, int index) {
                if (monitor != m_monitor)
                    return;
                m_state = state;
                m_index = index;
                emit stateChanged();
            });
}

void PlaylistBridge::load(const QString &monitor)
{
    if (monitor.isEmpty() || monitor == m_monitor)
        return;
    m_monitor = monitor;
    emit monitorChanged();
    reload();
}

void PlaylistBridge::reload()
{
    if (!m_playlist || m_monitor.isEmpty())
        return;

    auto *watcher = new QDBusPendingCallWatcher(m_playlist->Get(m_monitor), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString> reply = *call;
                if (reply.isError())
                    emit failed(reply.error().message());
                else
                    applyDocument(reply.value());
                call->deleteLater();
            });

    auto *stateWatcher = new QDBusPendingCallWatcher(m_playlist->State(m_monitor), this);
    connect(stateWatcher, &QDBusPendingCallWatcher::finished, this,
            [this](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString, int> reply = *call;
                if (!reply.isError()) {
                    m_state = reply.argumentAt<0>();
                    m_index = reply.argumentAt<1>();
                    emit stateChanged();
                }
                call->deleteLater();
            });
}

void PlaylistBridge::applyDocument(const QString &json)
{
    m_doc = json;
    m_entries.clear();

    const QJsonObject root = QJsonDocument::fromJson(json.toUtf8()).object();
    m_mode = root.value("mode").toString(QStringLiteral("sequential"));
    m_interval = root.value("intervalSec").toInt(60);
    m_loop = root.value("loop").toBool(true);

    for (const QJsonValue &value : root.value("entries").toArray()) {
        const QJsonObject entry = value.toObject();
        QVariantMap row;
        row[QStringLiteral("wallpaperId")] = entry.value("wallpaperId").toString();
        row[QStringLiteral("durationSec")] = entry.value("durationSec").toDouble();
        m_entries.push_back(row);
    }
    emit entriesChanged();
}

void PlaylistBridge::pushDocument()
{
    if (!m_playlist || m_monitor.isEmpty())
        return;

    QJsonArray entries;
    for (const QVariant &value : m_entries) {
        const QVariantMap row = value.toMap();
        QJsonObject entry;
        entry["wallpaperId"] = row.value(QStringLiteral("wallpaperId")).toString();
        entry["argv"] = QJsonArray::fromStringList(row.value(QStringLiteral("argv")).toStringList());
        entry["env"] = QJsonArray::fromStringList(row.value(QStringLiteral("env")).toStringList());
        entry["durationSec"] = row.value(QStringLiteral("durationSec")).toDouble();
        entries.append(entry);
    }

    QJsonObject root;
    root["schema"] = 1;
    root["mode"] = m_mode;
    root["intervalSec"] = m_interval;
    root["loop"] = m_loop;
    root["currentIndex"] = 0;
    root["entries"] = entries;

    const QString document = QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
    m_doc = document;
    m_playlist->Set(m_monitor, document);
}

void PlaylistBridge::add(const QVariantMap &entry)
{
    if (m_monitor.isEmpty())
        return;
    m_entries.push_back(entry);
    emit entriesChanged();
    pushDocument();
}

void PlaylistBridge::move(int from, int to)
{
    if (from < 0 || from >= m_entries.size() || to < 0 || to >= m_entries.size() || from == to)
        return;
    m_entries.move(from, to);
    emit entriesChanged();
    pushDocument();
}

void PlaylistBridge::removeAt(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    m_entries.removeAt(row);
    emit entriesChanged();
    pushDocument();
}

void PlaylistBridge::play()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Play(m_monitor);
}

void PlaylistBridge::pause()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Pause(m_monitor);
}

void PlaylistBridge::next()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Next(m_monitor);
}

void PlaylistBridge::prev()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Prev(m_monitor);
}

void PlaylistBridge::stop()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Stop(m_monitor);
}
