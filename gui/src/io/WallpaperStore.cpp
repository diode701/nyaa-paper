#include "io/WallpaperStore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace nyaa::io {

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

WallpaperStore::Last WallpaperStore::load(const std::string &monitor) const
{
    Last last;

    QFile file(QString::fromStdString(path_));
    if (!file.open(QIODevice::ReadOnly))
        return last;

    const QJsonObject root = QJsonDocument::fromJson(file.readAll()).object();
    const QJsonObject entry = root.value(QString::fromStdString(monitor)).toObject();
    last.wallpaperId = entry.value("wallpaperId").toString().toStdString();
    const QJsonObject values = entry.value("values").toObject();
    for (auto it = values.begin(); it != values.end(); ++it)
        last.values[it.key().toStdString()] = it.value().toString().toStdString();
    return last;
}

bool WallpaperStore::save(const std::string &monitor, const Last &last) const
{
    QJsonObject root;

    QFile file(QString::fromStdString(path_));
    if (file.open(QIODevice::ReadOnly))
        root = QJsonDocument::fromJson(file.readAll()).object();

    QJsonObject values;
    for (const auto &[key, value] : last.values)
        values[QString::fromStdString(key)] = QString::fromStdString(value);

    QJsonObject entry;
    entry["wallpaperId"] = QString::fromStdString(last.wallpaperId);
    entry["values"] = values;
    root[QString::fromStdString(monitor)] = entry;

    const QString target = QString::fromStdString(path_);
    QDir().mkpath(QFileInfo(target).absolutePath());
    QSaveFile out(target);
    if (!out.open(QIODevice::WriteOnly))
        return false;
    const QByteArray payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (out.write(payload) != payload.size())
        return false;
    return out.commit();
}

} // namespace nyaa::io
