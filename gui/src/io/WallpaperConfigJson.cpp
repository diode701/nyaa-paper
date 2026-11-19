#include "io/WallpaperConfigJson.h"

namespace nyaa::io {

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

} // namespace nyaa::io
