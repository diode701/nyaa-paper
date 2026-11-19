#pragma once

#include "IEngine.hpp"

class OrgNyaaPaperEngineInterface;

/// Talks to nyaa-paperd over the session bus.
///
/// Every call is asynchronous (QDBusPendingCallWatcher), so the event loop never
/// stalls waiting for the daemon. Failures surface as daemonError() rather than
/// as exceptions, because there is nobody to catch them.
class DbusEngine final : public IEngine
{
    Q_OBJECT

public:
    explicit DbusEngine(QObject *parent = nullptr);

    void refreshRunning() override;

public Q_SLOTS:
    void apply(const QString &monitor, const EngineRequest &command) override;
    void stop(const QString &monitor) override;
    void stopAll() override;

private:
    OrgNyaaPaperEngineInterface *m_engine = nullptr;
};
