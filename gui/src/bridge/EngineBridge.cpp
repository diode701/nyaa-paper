#include "EngineBridge.hpp"

#include "engine/DbusEngine.hpp"
#include "engine/MockEngine.hpp"

#include <QGuiApplication>
#include <QScreen>

void EngineBridge::applyWallpaper(const QString &monitor, const QString &wallpaperId)
{
    EngineCommand command;
    command.wallpaperId = wallpaperId;
    command.argv = {QStringLiteral("--screen-root"), monitor,
                    QStringLiteral("--bg"), wallpaperId};
    m_engine->apply(monitor, command);
}

EngineBridge::EngineBridge(QObject *parent)
    : QObject(parent)
{
    for (const QScreen *screen : QGuiApplication::screens()) {
        const QString name = screen->name();
        if (!name.isEmpty())
            m_monitors << name;
    }
    if (m_monitors.isEmpty())
        m_monitors = {QStringLiteral("eDP-1"), QStringLiteral("HDMI-1")};

    m_mock = qEnvironmentVariableIsSet("NYAA_MOCK");
    if (m_mock)
        m_engine = std::make_unique<MockEngine>();
    else
        m_engine = std::make_unique<DbusEngine>();

    connect(m_engine.get(), &IEngine::infoChanged, this, &EngineBridge::infoChanged);
    connect(m_engine.get(), &IEngine::engineExited, this, &EngineBridge::engineExited);
    connect(m_engine.get(), &IEngine::daemonError, this, &EngineBridge::error);
    connect(m_engine.get(), &IEngine::runningChanged, this, [this](const QMap<QString, int> &running) {
        m_running.clear();
        for (auto it = running.cbegin(); it != running.cend(); ++it)
            m_running.insert(it.key(), it.value());
        emit runningChanged();
    });
}
