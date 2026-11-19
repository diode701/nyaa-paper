#pragma once

#include <QObject>
#include <QStringList>

#include "io/WallpaperStore.h"

class ConfigBridge;
class EngineBridge;
class WallpaperModel;

/// WallpaperBrowser.autoRestoreWallpapers: at launch, put each monitor's
/// current wallpaper back, with its saved values and GPU.
///
/// Two processes change one thing: the daemon may still be running from
/// before, so monitors that already show a wallpaper are left alone. It runs
/// once, when both the library and the daemon's running list are known.
class StartupRestore : public QObject
{
    Q_OBJECT

public:
    StartupRestore(const ConfigBridge &config, EngineBridge &engine, const WallpaperModel &library,
                   QObject *parent = nullptr);

    /// The library has its first real contents.
    void libraryLoaded();

Q_SIGNALS:
    /// Which monitors were restored (empty when nothing needed it).
    void restored(const QStringList &monitors);

private:
    void tryRun();

    const ConfigBridge &m_config;
    EngineBridge &m_engine;
    const WallpaperModel &m_library;
    nyaa::io::WallpaperStore m_store;
    bool m_libraryReady = false;
    bool m_runningKnown = false;
    bool m_done = false;
};
