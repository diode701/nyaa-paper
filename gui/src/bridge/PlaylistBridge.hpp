#pragma once

#include <QJsonObject>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class ConfigBridge;
class OrgNyaaPaperPlaylistInterface;
class WallpaperModel;

/// The client's side of the daemon's playlists. Java equivalent: what
/// PlaylistManagerView and MiniPlaylistPanel did through PlaylistManager and
/// PlaylistRunner.
///
/// One monitor's playlist is "loaded" for the views. Every entry is kept whole
/// (its WallpaperConfig, argv, env, duration) and written back whole, and its
/// argv/env are re-rendered from the config on every save.
class PlaylistBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString monitor READ monitor NOTIFY monitorChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY entriesChanged)          // SEQUENTIAL | RANDOM
    Q_PROPERTY(int interval READ interval NOTIFY entriesChanged)
    Q_PROPERTY(bool loop READ loop NOTIFY entriesChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)          // idle | playing | paused
    Q_PROPERTY(int index READ index NOTIFY stateChanged)
    Q_PROPERTY(QStringList monitors READ monitors NOTIFY monitorsChanged)
    /// The runner's last message for this monitor ("Playing", "End of playlist"…)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)

public:
    PlaylistBridge(const WallpaperModel &library, const ConfigBridge &config, QObject *parent = nullptr);

    QString monitor() const { return m_monitor; }
    QVariantList entries() const;
    QString mode() const { return m_mode; }
    int interval() const { return m_interval; }
    bool loop() const { return m_loop; }
    QString state() const { return m_state; }
    int index() const { return m_index; }
    QStringList monitors() const { return m_monitors; }
    QString message() const { return m_message; }

    /// Which monitor the views show. Reloads even if it is the same one.
    Q_INVOKABLE void load(const QString &monitor);
    Q_INVOKABLE void reload();
    /// Java addToPlaylist: append to THAT monitor's playlist (loaded first),
    /// whatever the views show. Returns the new entry count, -1 on failure.
    Q_INVOKABLE int addTo(const QString &monitor, const QVariantMap &entry);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void move(int from, int to);
    Q_INVOKABLE void setMode(const QString &mode);
    Q_INVOKABLE void setInterval(int seconds);
    Q_INVOKABLE void setLoop(bool loop);
    Q_INVOKABLE void deletePlaylist();

    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    /// The Pause button: pause when playing, resume when paused.
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void prev();
    Q_INVOKABLE void reset();

    /// PropertiesView.applyToMonitor: a manual apply pauses that monitor's
    /// playlist first. True when it was playing and is now paused.
    bool pauseIfPlaying(const QString &monitor);

    /// Monitors with a stored playlist plus the connected ones (Java listed
    /// only the stored ones; a connected screen with none is useful too).
    void setConnectedMonitors(const QStringList &connected);

Q_SIGNALS:
    void monitorChanged();
    void entriesChanged();
    void stateChanged();
    void monitorsChanged();
    void messageChanged();
    void failed(const QString &message);

private:
    struct Document {
        QString mode = QStringLiteral("SEQUENTIAL");
        int interval = 60;
        bool loop = true;
        int currentIndex = 0;
        QList<QJsonObject> entries;
    };

    static Document parse(const QString &json);
    QString serialize(const Document &doc) const;
    /// Rebuild argv/env of an entry from its stored WallpaperConfig.
    QJsonObject rendered(const QJsonObject &entry, const QString &monitor) const;
    bool fetch(const QString &monitor, Document &out) const;   // blocking Get
    void push();                                               // Set for m_monitor
    void apply(const Document &doc);
    void refreshMonitors();

    const WallpaperModel &m_library;
    const ConfigBridge &m_config;
    OrgNyaaPaperPlaylistInterface *m_playlist = nullptr;

    QString m_monitor;
    QList<QJsonObject> m_entries;
    QString m_mode = QStringLiteral("SEQUENTIAL");
    int m_interval = 60;
    bool m_loop = true;
    int m_currentIndex = 0;
    QString m_state = QStringLiteral("idle");
    int m_index = -1;
    QStringList m_stored;
    QStringList m_connected;
    QStringList m_monitors;
    QString m_message;
    /// Bumped by every local edit. A reload answer that was asked for before
    /// the latest edit is stale and must not overwrite it.
    quint64 m_edits = 0;
    /// The daemon may run D-Bus calls concurrently, so two Sets in a row can
    /// land out of order. Only one is in flight; edits made meanwhile are
    /// sent, as the latest state, when it returns.
    bool m_saving = false;
    bool m_dirty = false;
};
