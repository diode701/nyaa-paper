#pragma once

#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

/// One engine invocation, fully rendered by the client.
///
/// This is the whole "how to play" instruction: the daemon spawns exactly this
/// and never interprets it. Java equivalent: the argument list that
/// WallpaperInstance used to build internally.
struct EngineRequest
{
    QString wallpaperId;
    QStringList argv;
    QStringList env; // "KEY=VALUE", matching the wire contract
};

/// The boundary between the UI and whatever owns the wallpaper processes.
///
/// Two implementations exist: DbusEngine (talks to nyaa-paperd) and MockEngine
/// (no daemon needed, for UI work and for demos on machines without Rust).
/// Nothing above this interface may know that D-Bus exists.
class IEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version READ version NOTIFY infoChanged)
    Q_PROPERTY(QString enginePath READ enginePath NOTIFY infoChanged)

public:
    explicit IEngine(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IEngine() override = default;

    QString version() const { return m_version; }
    QString enginePath() const { return m_enginePath; }

    /// Ask the daemon for the current monitor -> pid map; the answer arrives as
    /// runningChanged(). Async on purpose: never block the GUI thread on the bus.
    Q_INVOKABLE virtual void refreshRunning() = 0;

public Q_SLOTS:
    virtual void apply(const QString &monitor, const EngineRequest &command) = 0;
    virtual void stop(const QString &monitor) = 0;
    virtual void stopAll() = 0;

Q_SIGNALS:
    void infoChanged();
    void runningChanged(const QMap<QString, int> &running);
    void engineExited(const QString &monitor, int code);
    void daemonError(const QString &message);
    /// The answer to apply(): ok, or the daemon's error text.
    void applyFinished(const QString &monitor, bool ok, const QString &message);

protected:
    void setInfo(const QString &version, const QString &enginePath)
    {
        m_version = version;
        m_enginePath = enginePath;
        emit infoChanged();
    }

private:
    QString m_version = QStringLiteral("unknown");
    QString m_enginePath;
};
