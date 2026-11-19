#pragma once

#include "IEngine.hpp"

#include <QTimer>

/// A daemon that isn't there. Used for UI work and for demos on machines
/// without Rust or D-Bus.
class MockEngine final : public IEngine
{
    Q_OBJECT

public:
    explicit MockEngine(QObject *parent = nullptr)
        : IEngine(parent)
    {
        setInfo(QStringLiteral("0.1.0-mock"), QStringLiteral("/usr/bin/linux-wallpaperengine"));
    }

    void refreshRunning() override
    {
        QTimer::singleShot(60, this, [this] {
            QMap<QString, int> running{{QStringLiteral("eDP-1"), 4242}};
            Q_EMIT runningChanged(running);
        });
    }

public Q_SLOTS:
    void apply(const QString &monitor, const EngineRequest &command) override
    {
        Q_UNUSED(command);
        QTimer::singleShot(0, this, [this, monitor] { Q_EMIT applyFinished(monitor, true, {}); });
    }
    void stop(const QString &monitor) override { Q_UNUSED(monitor); }
    void stopAll() override {}
};
