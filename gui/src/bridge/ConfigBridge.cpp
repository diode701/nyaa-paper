#include "ConfigBridge.hpp"

#include <QCoreApplication>

#include "app/Autostart.hpp"

#include <QDBusConnection>
#include <QDBusMessage>


namespace {
QString toQ(const std::string &value) { return QString::fromStdString(value); }
std::string toS(const QString &value) { return value.toStdString(); }
} // namespace

ConfigBridge::ConfigBridge(QObject *parent)
    : QObject(parent)
    , m_config(m_store.load())
{
}

QString ConfigBridge::workshopPath() const { return toQ(m_config.paths.workshop); }
QString ConfigBridge::enginePath() const { return toQ(m_config.paths.engine); }
QString ConfigBridge::assetsPath() const { return toQ(m_config.paths.assets); }
QString ConfigBridge::screenshotsPath() const { return toQ(m_config.paths.screenshots); }
QString ConfigBridge::pywalPath() const { return toQ(m_config.paths.pywal); }
QString ConfigBridge::animation() const { return toQ(m_config.preview.animation); }
int ConfigBridge::pageSlideMs() const { return m_config.preview.pageSlideMs; }
QString ConfigBridge::layer() const { return toQ(m_config.engine.layer); }

void ConfigBridge::setWorkshopPath(const QString &value)
{
    m_config.paths.workshop = toS(value);
    emit changed();
}
void ConfigBridge::setEnginePath(const QString &value)
{
    m_config.paths.engine = toS(value);
    emit changed();
}
void ConfigBridge::setAssetsPath(const QString &value)
{
    m_config.paths.assets = toS(value);
    emit changed();
}
void ConfigBridge::setScreenshotsPath(const QString &value)
{
    m_config.paths.screenshots = toS(value);
    emit changed();
}
void ConfigBridge::setPywalPath(const QString &value)
{
    m_config.paths.pywal = toS(value);
    emit changed();
}
void ConfigBridge::setAnimation(const QString &value)
{
    m_config.preview.animation = toS(value);
    emit changed();
}
void ConfigBridge::setPageSlideMs(int value)
{
    m_config.preview.pageSlideMs = value;
    emit changed();
}
void ConfigBridge::setLayer(const QString &value)
{
    m_config.engine.layer = toS(value);
    emit changed();
}

void ConfigBridge::setPersistOnStart(bool on)
{
    m_config.startup.persistOnStart = on;
    emit changed();
}

void ConfigBridge::setAutoStartOnBoot(bool on)
{
    m_config.startup.autoStartOnBoot = on;
    emit changed();
}

QString ConfigBridge::customMonitors() const
{
    QStringList names;
    for (const auto &name : m_config.ui.customMonitors)
        names << toQ(name);
    return names.join(QStringLiteral(", "));
}

void ConfigBridge::setCustomMonitors(const QString &commaList)
{
    // Settings.getMonitors(): split on commas, trim, drop empties
    m_config.ui.customMonitors.clear();
    for (const QString &part : commaList.split(QLatin1Char(','), Qt::SkipEmptyParts))
        if (!part.trimmed().isEmpty())
            m_config.ui.customMonitors.push_back(toS(part.trimmed()));
    emit changed();
}

bool ConfigBridge::save()
{
    const bool ok = m_store.save(m_config);
    // NyaaLauncher.saveAndLaunch → applyAutoStart(autoStartOnBoot)
    QString error;
    if (!Autostart::apply(m_config.startup.autoStartOnBoot, Autostart::standardPaths(),
                          QCoreApplication::applicationFilePath(), Autostart::runningDaemonPath(), &error))
        emit problem(tr("Autostart setup failed: %1").arg(error));
    emit saved(ok);
    return ok;
}

void ConfigBridge::reload()
{
    m_config = m_store.load();
    emit changed();
}

QString ConfigBridge::filePath() const
{
    return QString::fromStdString(m_store.path());
}




void ConfigBridge::rescanLibrary()
{
    const QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("org.nyaa.Paper"), QStringLiteral("/org/nyaa/Paper"),
        QStringLiteral("org.nyaa.Paper.Library"), QStringLiteral("Rescan"));
    QDBusConnection::sessionBus().send(call);
}


