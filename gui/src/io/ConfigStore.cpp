#include "io/ConfigStore.h"

#include <cstdlib>
#include <fstream>
#include <sstream>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>

namespace nyaa::io {

namespace {

QJsonObject toJson(const AppConfig &config)
{
    QJsonObject paths;
    paths["workshop"] = QString::fromStdString(config.paths.workshop);
    paths["engine"] = QString::fromStdString(config.paths.engine);
    paths["assets"] = QString::fromStdString(config.paths.assets);
    paths["screenshots"] = QString::fromStdString(config.paths.screenshots);
    paths["pywal"] = QString::fromStdString(config.paths.pywal);

    QJsonObject preview;
    preview["animation"] = QString::fromStdString(config.preview.animation);
    preview["pageSlideMs"] = config.preview.pageSlideMs;
    preview["cachePages"] = config.preview.cachePages;

    QJsonObject engine;
    engine["layer"] = QString::fromStdString(config.engine.layer);

    QJsonObject ui;
    ui["monitor"] = QString::fromStdString(config.ui.monitor);
    QJsonArray custom;
    for (const auto &name : config.ui.customMonitors)
        custom.append(QString::fromStdString(name));
    ui["customMonitors"] = custom;

    QJsonObject startup;
    startup["persistOnStart"] = config.startup.persistOnStart;
    startup["autoStartOnBoot"] = config.startup.autoStartOnBoot;

    QJsonObject root;
    root["version"] = config.version;
    root["paths"] = paths;
    root["preview"] = preview;
    root["engine"] = engine;
    root["ui"] = ui;
    root["startup"] = startup;
    return root;
}

std::string stringOr(const QJsonObject &object, const char *key, const std::string &fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isString() ? value.toString().toStdString() : fallback;
}

int intOr(const QJsonObject &object, const char *key, int fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isDouble() ? value.toInt() : fallback;
}

bool boolOr(const QJsonObject &object, const char *key, bool fallback)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isBool() ? value.toBool() : fallback;
}

AppConfig fromJson(const QJsonObject &root)
{
    AppConfig config;

    const QJsonObject paths = root.value("paths").toObject();
    if (paths.contains("workshop"))
        config.paths.workshop = stringOr(paths, "workshop", config.paths.workshop);
    config.paths.engine = stringOr(paths, "engine", config.paths.engine);
    config.paths.assets = stringOr(paths, "assets", config.paths.assets);
    config.paths.screenshots = stringOr(paths, "screenshots", config.paths.screenshots);
    config.paths.pywal = stringOr(paths, "pywal", config.paths.pywal);

    const QJsonObject preview = root.value("preview").toObject();
    config.preview.animation = stringOr(preview, "animation", config.preview.animation);
    config.preview.pageSlideMs = intOr(preview, "pageSlideMs", config.preview.pageSlideMs);
    config.preview.cachePages = intOr(preview, "cachePages", config.preview.cachePages);

    const QJsonObject engine = root.value("engine").toObject();
    config.engine.layer = stringOr(engine, "layer", config.engine.layer);

    const QJsonObject ui = root.value("ui").toObject();
    config.ui.monitor = stringOr(ui, "monitor", config.ui.monitor);
    for (const QJsonValue &name : ui.value("customMonitors").toArray())
        if (!name.toString().trimmed().isEmpty())
            config.ui.customMonitors.push_back(name.toString().trimmed().toStdString());

    const QJsonObject startup = root.value("startup").toObject();
    config.startup.persistOnStart = boolOr(startup, "persistOnStart", config.startup.persistOnStart);
    config.startup.autoStartOnBoot = boolOr(startup, "autoStartOnBoot", config.startup.autoStartOnBoot);

    return config;
}

} // namespace

std::string expandUser(const std::string &path)
{
    if (!path.empty() && path[0] == '~') {
        const char *home = std::getenv("HOME");
        if (home)
            return std::string(home) + path.substr(1);
    }
    if (path.rfind("$HOME", 0) == 0) {
        const char *home = std::getenv("HOME");
        if (home)
            return std::string(home) + path.substr(5);
    }
    return path;
}

ConfigStore::ConfigStore()
    : path_(defaultPath())
{
}

ConfigStore::ConfigStore(std::string path)
    : path_(std::move(path))
{
}

std::string ConfigStore::defaultPath()
{
    QString dir;
    const QString xdg = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (!xdg.isEmpty())
        dir = xdg + "/nyaa-paper";
    else
        dir = QDir::homePath() + "/.config/nyaa-paper";
    return (dir + "/config.json").toStdString();
}

std::string ConfigStore::detectWorkshopPath()
{
    // The same rule as the daemon (paths.rs) and Java's Builder: the env
    // override, else native Steam's default. Flatpak Steam is out of scope.
    const QString overridden = qEnvironmentVariable("NYAA_WORKSHOP");
    if (!overridden.isEmpty())
        return overridden.toStdString();
    return (QDir::homePath() + "/.local/share/Steam/steamapps/workshop/content/431960").toStdString();
}

AppConfig ConfigStore::load() const
{
    AppConfig config;

    // Defaults that are discovered rather than typed by the user.
    config.paths.workshop = detectWorkshopPath();
    if (config.paths.screenshots.empty())
        config.paths.screenshots = QDir::homePath().toStdString() + "/Pictures";
    if (config.paths.pywal.empty())
        config.paths.pywal = QDir::homePath().toStdString() + "/.cache/wal";

    QFile file(QString::fromStdString(path_));
    if (!file.exists()) {
        save(config);   // create it once so the file is discoverable
        return config;
    }
    if (!file.open(QIODevice::ReadOnly))
        return config;

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject())
        return config;   // malformed: keep defaults, leave the file for inspection

    return fromJson(document.object());
}

bool ConfigStore::save(const AppConfig &config) const
{
    const QString target = QString::fromStdString(path_);
    QDir().mkpath(QFileInfo(target).absolutePath());

    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    const QByteArray payload = QJsonDocument(toJson(config)).toJson(QJsonDocument::Indented);
    if (file.write(payload) != payload.size())
        return false;
    return file.commit();
}

} // namespace nyaa::io
