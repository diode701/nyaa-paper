#include "ConfigRenderer.hpp"

#include "domain/Renderer.h"
#include "domain/options/GlobalOptions.h"
#include "domain/settings/SettingFactory.h"
#include "domain/settings/SettingList.h"
#include "io/ProjectJson.h"

namespace ConfigRenderer {

EngineRequest render(const nyaa::domain::WallpaperConfig &config, const QString &basePath,
                     const nyaa::io::AppConfig &app)
{
    using namespace nyaa::domain;

    Wallpaper wallpaper;
    wallpaper.id = config.wallpaperId;
    wallpaper.basePath = basePath.toStdString();

    settings::SettingList list;
    for (const auto &desc : nyaa::io::parseProjectFile(wallpaper.basePath + "/project.json"))
        list.add(settings::makeSetting(desc));
    list.restore(config.settings);

    options::GlobalOptions globals;
    globals.restore(config.settings);

    EngineOptions options;
    options.workshopRoot = app.paths.workshop;
    options.assetsDir = app.paths.assets;
    options.layer = app.engine.layer;

    // Java's playlist applies without a screenshot: apply(..., null, 0)
    const EngineCommand command = Renderer::render(config.monitor, wallpaper, list, globals,
                                                   config.gpu, Screenshot{}, options);
    EngineRequest request;
    request.wallpaperId = QString::fromStdString(config.wallpaperId);
    for (const auto &arg : command.argv)
        request.argv << QString::fromStdString(arg);
    for (const auto &entry : command.env)
        request.env << QString::fromStdString(entry);
    return request;
}

} // namespace ConfigRenderer
