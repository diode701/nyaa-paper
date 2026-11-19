#include "ConfigBridge.hpp"

#include <QDBusConnection>
#include <QDBusMessage>

#include <algorithm>

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
int ConfigBridge::volume() const { return m_config.engine.volume; }
bool ConfigBridge::muted() const { return m_config.engine.muted; }
bool ConfigBridge::fpsLimited() const { return m_config.engine.fpsLimited; }
int ConfigBridge::fps() const { return m_config.engine.fps; }
QString ConfigBridge::scaling() const { return toQ(m_config.engine.scaling); }
QString ConfigBridge::clampMode() const { return toQ(m_config.engine.clamp); }
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
void ConfigBridge::setVolume(int value)
{
    m_config.engine.volume = value;
    emit changed();
}
void ConfigBridge::setMuted(bool value)
{
    m_config.engine.muted = value;
    emit changed();
}
void ConfigBridge::setFpsLimited(bool value)
{
    m_config.engine.fpsLimited = value;
    emit changed();
}
void ConfigBridge::setFps(int value)
{
    m_config.engine.fps = value;
    emit changed();
}
void ConfigBridge::setScaling(const QString &value)
{
    m_config.engine.scaling = toS(value);
    emit changed();
}
void ConfigBridge::setClampMode(const QString &value)
{
    m_config.engine.clamp = toS(value);
    emit changed();
}
void ConfigBridge::setLayer(const QString &value)
{
    m_config.engine.layer = toS(value);
    emit changed();
}

bool ConfigBridge::save()
{
    const bool ok = m_store.save(m_config);
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

bool ConfigBridge::flagEnabled(const QString &name) const
{
    const std::string key = name.toStdString();
    for (const auto &flag : m_config.engine.flags) {
        if (flag == key)
            return true;
    }
    return false;
}

void ConfigBridge::setFlag(const QString &name, bool enabled)
{
    const std::string key = name.toStdString();
    auto &flags = m_config.engine.flags;
    const auto it = std::find(flags.begin(), flags.end(), key);
    if (enabled && it == flags.end())
        flags.push_back(key);
    else if (!enabled && it != flags.end())
        flags.erase(it);
    emit changed();
}

void ConfigBridge::rescanLibrary()
{
    const QDBusMessage call = QDBusMessage::createMethodCall(
        QStringLiteral("org.nyaa.Paper"), QStringLiteral("/org/nyaa/Paper"),
        QStringLiteral("org.nyaa.Paper.Library"), QStringLiteral("Rescan"));
    QDBusConnection::sessionBus().send(call);
}

bool ConfigBridge::saveApplied(const QString &monitor, const QString &wallpaperId,
                               const QVariantMap &values)
{
    nyaa::io::WallpaperStore::Last last;
    last.wallpaperId = wallpaperId.toStdString();
    for (auto it = values.begin(); it != values.end(); ++it)
        last.values[it.key().toStdString()] = it.value().toString().toStdString();
    return m_wallpapers.save(monitor.toStdString(), last);
}

QVariantMap ConfigBridge::loadApplied(const QString &monitor)
{
    const nyaa::io::WallpaperStore::Last last = m_wallpapers.load(monitor.toStdString());
    QVariantMap values;
    for (const auto &[key, value] : last.values)
        values.insert(QString::fromStdString(key), QString::fromStdString(value));
    values.insert(QStringLiteral("__wallpaperId"), QString::fromStdString(last.wallpaperId));
    return values;
}
