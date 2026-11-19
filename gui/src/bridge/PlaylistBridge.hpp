#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

class OrgNyaaPaperPlaylistInterface;

/// The client's view of the daemon's playlist: entries, playback state, and the
/// add/remove operations that rewrite the stored document.
class PlaylistBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString monitor READ monitor NOTIFY monitorChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(int index READ index NOTIFY stateChanged)

public:
    explicit PlaylistBridge(QObject *parent = nullptr);

    QString monitor() const { return m_monitor; }
    QVariantList entries() const { return m_entries; }
    QString state() const { return m_state; }
    int index() const { return m_index; }

    Q_INVOKABLE void load(const QString &monitor);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void add(const QVariantMap &entry);
    Q_INVOKABLE void removeAt(int row);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void next();
    Q_INVOKABLE void prev();

Q_SIGNALS:
    void monitorChanged();
    void entriesChanged();
    void stateChanged();
    void failed(const QString &message);

private:
    void applyDocument(const QString &json);
    void pushDocument();

    OrgNyaaPaperPlaylistInterface *m_playlist = nullptr;
    QString m_monitor;
    QVariantList m_entries;
    QString m_doc;          // the stored document, kept as text and re-written
    QString m_mode = QStringLiteral("sequential");
    int m_interval = 60;
    bool m_loop = true;
    QString m_state = QStringLiteral("idle");
    int m_index = -1;
};
