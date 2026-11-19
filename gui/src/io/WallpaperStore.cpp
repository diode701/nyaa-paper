#include "io/WallpaperStore.h"
#include "io/WallpaperConfigJson.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace nyaa::io {

namespace {

const QString kCurrent = QStringLiteral("current");
const QString kConfigs = QStringLiteral("configs");

/// One monitor's block, upgrading the one-record layout of earlier builds
/// ({wallpaperId, settings, ...} directly under the monitor).
QJsonObject monitorBlock(const QJsonObject &root, const QString &monitor)
{
    const QJsonObject block = root.value(monitor).toObject();
    if (block.contains(kConfigs) || block.isEmpty())
        return block;
    const QString id = block.value("wallpaperId").toString();
    if (id.isEmpty())
        return {};
    return QJsonObject{{kCurrent, id}, {kConfigs, QJsonObject{{id, block}}}};
}

domain::WallpaperConfig configIn(const QJsonObject &block, const QString &monitor, const QString &id)
{
    const QJsonObject json = block.value(kConfigs).toObject().value(id).toObject();
    if (json.isEmpty())
        return {};
    domain::WallpaperConfig config = fromJson(json);
    if (config.wallpaperId.empty())
        config.wallpaperId = id.toStdString();
    if (config.monitor.empty())
        config.monitor = monitor.toStdString();
    return config;
}

} // namespace

WallpaperStore::WallpaperStore()
    : path_(defaultPath())
{
}

WallpaperStore::WallpaperStore(std::string path)
    : path_(std::move(path))
{
}

std::string WallpaperStore::defaultPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (dir.isEmpty())
        dir = QDir::homePath() + "/.config";
    return (dir + "/nyaa-paper/wallpapers.json").toStdString();
}

namespace {

QJsonObject readRoot(const std::string &path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    const QByteArray raw = file.readAll();
    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(raw, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        // Java threw here and the app died at startup. Keep the evidence
        // instead, and carry on with an empty store.
        file.close();
        const QString backup = QString::fromStdString(path) + ".broken-"
                               + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss");
        QFile::copy(QString::fromStdString(path), backup);
        qWarning("wallpapers.json is not valid JSON; kept a copy at %s", qPrintable(backup));
        QFile::remove(QString::fromStdString(path));
        return {};
    }
    return document.object();
}

bool writeRoot(const std::string &path, const QJsonObject &root)
{
    const QString target = QString::fromStdString(path);
    QDir().mkpath(QFileInfo(target).absolutePath());
    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(payload) != payload.size())
        return false;
    return out.commit();
}

} // namespace

domain::WallpaperConfig WallpaperStore::load(const std::string &monitor, const std::string &wallpaperId) const
{
    const QString mon = QString::fromStdString(monitor);
    return configIn(monitorBlock(readRoot(path_), mon), mon, QString::fromStdString(wallpaperId));
}

domain::WallpaperConfig WallpaperStore::current(const std::string &monitor) const
{
    const QString mon = QString::fromStdString(monitor);
    const QJsonObject block = monitorBlock(readRoot(path_), mon);
    return configIn(block, mon, block.value(kCurrent).toString());
}

bool WallpaperStore::save(const domain::WallpaperConfig &config) const
{
    if (config.monitor.empty() || config.wallpaperId.empty())
        return false;
    const QString mon = QString::fromStdString(config.monitor);
    const QString id = QString::fromStdString(config.wallpaperId);

    QJsonObject root = readRoot(path_);
    QJsonObject block = monitorBlock(root, mon);
    QJsonObject configs = block.value(kConfigs).toObject();
    configs[id] = toJson(config);
    block[kConfigs] = configs;
    block[kCurrent] = id;
    root[mon] = block;
    return writeRoot(path_, root);
}

bool WallpaperStore::remove(const std::string &monitor, const std::string &wallpaperId) const
{
    const QString mon = QString::fromStdString(monitor);
    const QString id = QString::fromStdString(wallpaperId);

    QJsonObject root = readRoot(path_);
    QJsonObject block = monitorBlock(root, mon);
    QJsonObject configs = block.value(kConfigs).toObject();
    if (!configs.contains(id))
        return true;
    configs.remove(id);
    if (block.value(kCurrent).toString() == id)
        block.remove(kCurrent);
    if (configs.isEmpty()) {
        root.remove(mon);   // Java also removed the empty monitor directory
    } else {
        block[kConfigs] = configs;
        root[mon] = block;
    }
    return writeRoot(path_, root);
}

int WallpaperStore::cleanupStale(const std::set<std::string> &activeIds) const
{
    if (activeIds.empty())
        return 0;   // an unreadable library must never wipe the saved configs

    QJsonObject root = readRoot(path_);
    int removed = 0;
    for (const QString &mon : root.keys()) {
        QJsonObject block = monitorBlock(root, mon);
        QJsonObject configs = block.value(kConfigs).toObject();
        for (const QString &id : configs.keys()) {
            if (!activeIds.count(id.toStdString())) {
                configs.remove(id);
                ++removed;
                if (block.value(kCurrent).toString() == id)
                    block.remove(kCurrent);
            }
        }
        if (configs.isEmpty()) {
            root.remove(mon);
        } else {
            block[kConfigs] = configs;
            root[mon] = block;
        }
    }
    if (removed > 0)
        writeRoot(path_, root);
    return removed;
}

} // namespace nyaa::io
