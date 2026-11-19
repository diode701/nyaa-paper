#include "io/WallpaperStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace nyaa::io {

namespace {

QJsonObject readRoot(const std::string &path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(file.readAll()).object();
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

// WallpaperConfig.toJson
QJsonObject toJson(const domain::WallpaperConfig &config)
{
    QJsonObject settings;
    for (const auto &[key, value] : config.settings)
        settings[QString::fromStdString(key)] = QString::fromStdString(value);

    QJsonObject json;
    json["version"] = config.version;
    json["wallpaperId"] = QString::fromStdString(config.wallpaperId);
    json["monitor"] = QString::fromStdString(config.monitor);
    json["projectJsonModified"] = double(config.projectJsonModified);
    json["gpuMode"] = QString::fromStdString(domain::gpuModeName(config.gpu.mode));
    json["driPrimeNumber"] = config.gpu.driPrimeNumber;
    json["glThreadedOpt"] = config.gpu.glThreaded;
    json["settings"] = settings;
    return json;
}

// WallpaperConfig.fromJson, with the same defaults
domain::WallpaperConfig fromJson(const QJsonObject &json)
{
    domain::WallpaperConfig config;
    config.version = json.value("version").toInt(domain::WallpaperConfig::kCurrentVersion);
    config.wallpaperId = json.value("wallpaperId").toString().toStdString();
    config.monitor = json.value("monitor").toString().toStdString();
    config.projectJsonModified = static_cast<long long>(json.value("projectJsonModified").toDouble(0));
    config.gpu.mode = domain::gpuModeFromName(json.value("gpuMode").toString("DEFAULT").toStdString());
    config.gpu.driPrimeNumber = json.value("driPrimeNumber").toInt(1);
    config.gpu.glThreaded = json.value("glThreadedOpt").toBool(false);

    // "values" is what the first v2 builds wrote; read it so nothing is lost.
    QJsonObject settings = json.value("settings").toObject();
    if (settings.isEmpty())
        settings = json.value("values").toObject();
    for (auto it = settings.begin(); it != settings.end(); ++it)
        config.settings[it.key().toStdString()] = it.value().toString().toStdString();
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

domain::WallpaperConfig WallpaperStore::load(const std::string &monitor) const
{
    const QJsonObject entry = readRoot(path_).value(QString::fromStdString(monitor)).toObject();
    if (entry.isEmpty())
        return {};
    domain::WallpaperConfig config = fromJson(entry);
    if (config.monitor.empty())
        config.monitor = monitor;
    return config;
}

bool WallpaperStore::save(const domain::WallpaperConfig &config) const
{
    if (config.monitor.empty())
        return false;
    QJsonObject root = readRoot(path_);
    root[QString::fromStdString(config.monitor)] = toJson(config);
    return writeRoot(path_, root);
}

bool WallpaperStore::remove(const std::string &monitor) const
{
    QJsonObject root = readRoot(path_);
    if (!root.contains(QString::fromStdString(monitor)))
        return true;
    root.remove(QString::fromStdString(monitor));
    return writeRoot(path_, root);
}

int WallpaperStore::cleanupStale(const std::set<std::string> &activeIds) const
{
    QJsonObject root = readRoot(path_);
    int removed = 0;
    for (const QString &monitor : root.keys()) {
        const std::string id = root.value(monitor).toObject().value("wallpaperId").toString().toStdString();
        if (!activeIds.count(id)) {
            root.remove(monitor);
            ++removed;
        }
    }
    if (removed > 0)
        writeRoot(path_, root);
    return removed;
}

} // namespace nyaa::io
