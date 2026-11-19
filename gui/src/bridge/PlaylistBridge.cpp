#include "PlaylistBridge.hpp"

#include "playlist_proxy.h"

#include <QDBusConnection>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <QJsonArray>
#include <QJsonDocument>

#include "bridge/ConfigBridge.hpp"
#include "bridge/ConfigRenderer.hpp"
#include "io/WallpaperConfigJson.h"
#include "models/WallpaperModel.hpp"

namespace {
constexpr auto kService = "org.nyaa.Paper";
constexpr auto kPath = "/org/nyaa/Paper";
} // namespace

PlaylistBridge::PlaylistBridge(const WallpaperModel &library, const ConfigBridge &config, QObject *parent)
    : QObject(parent)
    , m_library(library)
    , m_config(config)
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
    // Java: PlaylistRunner's statusListener
    connect(m_playlist, &OrgNyaaPaperPlaylistInterface::Message, this,
            [this](const QString &monitor, const QString &text) {
                if (monitor != m_monitor)
                    return;
                m_message = text;
                emit messageChanged();
            });
    // Another view (or the runner pruning entries) changed a playlist.
    connect(m_playlist, &OrgNyaaPaperPlaylistInterface::PlaylistChanged, this,
            [this](const QString &monitor) {
                refreshMonitors();
                if (monitor == m_monitor)
                    reload();
            });
    refreshMonitors();
}

// ---- document <-> JSON -----------------------------------------------------

PlaylistBridge::Document PlaylistBridge::parse(const QString &json)
{
    Document doc;
    const QJsonObject root = QJsonDocument::fromJson(json.toUtf8()).object();
    // The wire format says "sequential"/"random"; the UI shows Java's names.
    doc.mode = root.value("mode").toString("sequential").toUpper() == QStringLiteral("RANDOM")
                   ? QStringLiteral("RANDOM") : QStringLiteral("SEQUENTIAL");
    doc.interval = root.value("intervalSec").toInt(60);
    doc.loop = root.value("loop").toBool(true);
    doc.currentIndex = root.value("currentIndex").toInt(0);
    for (const QJsonValue &value : root.value("entries").toArray())
        doc.entries.push_back(value.toObject());   // whole entry, nothing dropped
    return doc;
}

QString PlaylistBridge::serialize(const Document &doc) const
{
    QJsonArray entries;
    for (const QJsonObject &entry : doc.entries)
        entries.append(entry);

    QJsonObject root;
    root["schema"] = 1;
    root["mode"] = doc.mode.toLower();
    root["intervalSec"] = doc.interval;
    root["loop"] = doc.loop;
    root["currentIndex"] = doc.currentIndex;
    root["entries"] = entries;
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QJsonObject PlaylistBridge::rendered(const QJsonObject &entry, const QString &monitor) const
{
    const QJsonObject configJson = entry.value("config").toObject();
    if (configJson.isEmpty())
        return entry;   // written by an older build: keep its argv as it is

    nyaa::domain::WallpaperConfig config = nyaa::io::fromJson(configJson);
    config.monitor = monitor.toStdString();
    const QString basePath = m_library.basePathFor(QString::fromStdString(config.wallpaperId));
    if (basePath.isEmpty())
        return entry;   // not in the library right now: keep the last good argv

    const EngineRequest request = ConfigRenderer::render(config, basePath, m_config.config());
    QJsonObject out = entry;
    out["config"] = nyaa::io::toJson(config);
    out["argv"] = QJsonArray::fromStringList(request.argv);
    out["env"] = QJsonArray::fromStringList(request.env);
    return out;
}

// ---- talking to the daemon -------------------------------------------------

bool PlaylistBridge::fetch(const QString &monitor, Document &out) const
{
    if (!m_playlist)
        return false;
    QDBusPendingReply<QString> reply = m_playlist->Get(monitor);
    reply.waitForFinished();
    if (reply.isError())
        return false;
    out = parse(reply.value());
    return true;
}

void PlaylistBridge::apply(const Document &doc)
{
    m_entries = doc.entries;
    m_mode = doc.mode;
    m_interval = doc.interval;
    m_loop = doc.loop;
    m_currentIndex = doc.currentIndex;
    emit entriesChanged();
}

void PlaylistBridge::push()
{
    if (!m_playlist || m_monitor.isEmpty())
        return;
    Document doc;
    doc.mode = m_mode;
    doc.interval = m_interval;
    doc.loop = m_loop;
    doc.currentIndex = m_currentIndex;
    for (const QJsonObject &entry : m_entries)
        doc.entries.push_back(rendered(entry, m_monitor));
    m_entries = doc.entries;

    auto *watcher = new QDBusPendingCallWatcher(m_playlist->Set(m_monitor, serialize(doc)), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        const QDBusPendingReply<> reply = *call;
        if (reply.isError())
            emit failed(reply.error().message());
        call->deleteLater();
    });
}

void PlaylistBridge::refreshMonitors()
{
    if (m_playlist) {
        const QDBusReply<QStringList> reply = m_playlist->ListMonitors();
        if (reply.isValid())
            m_stored = reply.value();
    }
    QStringList all = m_stored;
    for (const QString &monitor : m_connected)
        if (!all.contains(monitor))
            all << monitor;
    if (all != m_monitors) {
        m_monitors = all;
        emit monitorsChanged();
    }
}

void PlaylistBridge::setConnectedMonitors(const QStringList &connected)
{
    m_connected = connected;
    refreshMonitors();
}

// ---- views -----------------------------------------------------------------

QVariantList PlaylistBridge::entries() const
{
    QVariantList out;
    for (const QJsonObject &entry : m_entries)
        out << entry.toVariantMap();
    return out;
}

void PlaylistBridge::load(const QString &monitor)
{
    if (monitor.isEmpty())
        return;
    if (monitor != m_monitor) {
        m_monitor = monitor;
        m_message.clear();
        emit monitorChanged();
        emit messageChanged();
    }
    reload();
}

void PlaylistBridge::reload()
{
    if (!m_playlist || m_monitor.isEmpty())
        return;

    const QString monitor = m_monitor;
    auto *watcher = new QDBusPendingCallWatcher(m_playlist->Get(monitor), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, monitor](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString> reply = *call;
                if (reply.isError())
                    emit failed(reply.error().message());
                else if (monitor == m_monitor)
                    apply(parse(reply.value()));
                call->deleteLater();
            });

    auto *stateWatcher = new QDBusPendingCallWatcher(m_playlist->State(monitor), this);
    connect(stateWatcher, &QDBusPendingCallWatcher::finished, this,
            [this, monitor](QDBusPendingCallWatcher *call) {
                const QDBusPendingReply<QString, int> reply = *call;
                if (!reply.isError() && monitor == m_monitor) {
                    m_state = reply.argumentAt<0>();
                    m_index = reply.argumentAt<1>();
                    emit stateChanged();
                }
                call->deleteLater();
            });
}

int PlaylistBridge::addTo(const QString &monitorName, const QVariantMap &entry)
{
    const QString monitor = monitorName.trimmed();
    if (!m_playlist || monitor.isEmpty())
        return -1;

    // Java: playlistManager.load(mon) → add → save(mon), for that monitor.
    Document doc;
    if (!fetch(monitor, doc))
        return -1;
    doc.entries.push_back(rendered(QJsonObject::fromVariantMap(entry), monitor));

    QDBusPendingReply<> reply = m_playlist->Set(monitor, serialize(doc));
    reply.waitForFinished();
    if (reply.isError()) {
        emit failed(reply.error().message());
        return -1;
    }
    if (monitor == m_monitor)
        apply(doc);
    refreshMonitors();
    return int(doc.entries.size());
}

void PlaylistBridge::removeAt(int row)
{
    if (row < 0 || row >= m_entries.size())
        return;
    m_entries.removeAt(row);
    // removeEntry: keep currentIndex inside the list
    m_currentIndex = qMax(0, qMin(m_currentIndex, int(m_entries.size()) - 1));
    emit entriesChanged();
    push();
}

void PlaylistBridge::move(int from, int to)
{
    if (from < 0 || from >= m_entries.size() || to < 0 || to >= m_entries.size() || from == to)
        return;
    m_entries.move(from, to);
    emit entriesChanged();
    push();
}

void PlaylistBridge::setMode(const QString &mode)
{
    m_mode = mode.toUpper() == QStringLiteral("RANDOM") ? QStringLiteral("RANDOM") : QStringLiteral("SEQUENTIAL");
    emit entriesChanged();
    push();
}

void PlaylistBridge::setInterval(int seconds)
{
    m_interval = qBound(5, seconds, 3600);   // the Java spinner's range
    emit entriesChanged();
    push();
}

void PlaylistBridge::setLoop(bool loop)
{
    m_loop = loop;
    emit entriesChanged();
    push();
}

void PlaylistBridge::deletePlaylist()
{
    if (!m_playlist || m_monitor.isEmpty())
        return;
    m_playlist->Delete(m_monitor).waitForFinished();
    apply(Document{});
    refreshMonitors();
}

bool PlaylistBridge::pauseIfPlaying(const QString &monitor)
{
    if (!m_playlist || monitor.isEmpty())
        return false;
    // Blocking on purpose: the apply must not race the playlist's next tick.
    int index = -1;
    const QDBusReply<QString> state = m_playlist->State(monitor, index);
    if (!state.isValid() || state.value() != QStringLiteral("playing"))
        return false;
    m_playlist->Pause(monitor).waitForFinished();
    return true;
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

void PlaylistBridge::togglePause()
{
    // Java: if (runner.isPaused(m)) runner.resume(m); else runner.pause(m);
    if (m_state == QStringLiteral("paused"))
        play();
    else
        pause();
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

void PlaylistBridge::reset()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Reset(m_monitor);
}

void PlaylistBridge::stop()
{
    if (m_playlist && !m_monitor.isEmpty())
        m_playlist->Stop(m_monitor);
}
