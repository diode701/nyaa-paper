#include "StartupRestore.hpp"

#include "bridge/ConfigBridge.hpp"
#include "bridge/ConfigRenderer.hpp"
#include "bridge/EngineBridge.hpp"
#include "models/WallpaperModel.hpp"

StartupRestore::StartupRestore(const ConfigBridge &config, EngineBridge &engine,
                               const WallpaperModel &library, QObject *parent)
    : QObject(parent)
    , m_config(config)
    , m_engine(engine)
    , m_library(library)
{
    connect(&m_engine, &EngineBridge::runningChanged, this, [this] {
        m_runningKnown = true;
        tryRun();
    });
    m_engine.refreshRunning();
}

void StartupRestore::libraryLoaded()
{
    m_libraryReady = true;
    tryRun();
}

void StartupRestore::tryRun()
{
    if (m_done || !m_libraryReady || !m_runningKnown)
        return;
    m_done = true;
    if (!m_config.persistOnStart())
        return;

    QStringList monitors;
    const QVariantMap running = m_engine.running();
    for (const QString &monitor : m_engine.monitors()) {   // Java: settings.getMonitors()
        if (running.contains(monitor))
            continue;   // the daemon kept it running; nothing to restore
        const nyaa::domain::WallpaperConfig config = m_store.current(monitor.toStdString());
        if (config.empty())
            continue;
        const QString basePath = m_library.basePathFor(QString::fromStdString(config.wallpaperId));
        if (basePath.isEmpty())
            continue;   // unsubscribed; cleanupStale drops it
        m_engine.apply(monitor, ConfigRenderer::render(config, basePath, m_config.config()));
        monitors << monitor;
    }
    emit restored(monitors);
}
