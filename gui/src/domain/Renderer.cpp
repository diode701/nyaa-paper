#include "domain/Renderer.h"

#include <format>
#include <vector>

namespace nyaa::domain {

namespace {
void appendScreenshot(std::vector<std::string> &argv, const std::string &monitor,
                      const Screenshot &screenshot)
{
    if (!screenshot.enabled())
        return;
    std::string dir = screenshot.directory;
    if (!dir.empty() && dir.back() != '/')
        dir.push_back('/');
    argv.push_back("--screenshot");
    argv.push_back(dir + monitor + "_nyaa_sc.png");   // same name as the Java app
    argv.push_back("--screenshot-delay");
    argv.push_back(std::to_string(screenshot.delay));
}

void appendEnvironment(std::vector<std::string> &env, const GpuConfig &gpu)
{
    switch (gpu.mode) {
    case GpuConfig::Mode::DriPrime:
        env.push_back(std::format("DRI_PRIME={}", gpu.driPrimeNumber));
        break;
    case GpuConfig::Mode::Nvidia:
        env.push_back("__NV_PRIME_RENDER_OFFLOAD=1");
        env.push_back("__GLX_VENDOR_LIBRARY_NAME=nvidia");
        break;
    case GpuConfig::Mode::Default:
        break;
    }
    if (gpu.glThreaded)
        env.push_back("__GL_THREADED_OPTIMIZATIONS=0");
}
} // namespace

namespace {
/// An id lets linux-wallpaperengine find the wallpaper through Steam's standard
/// directories. Anywhere else it must be handed the folder itself.
std::string backgroundArgument(const Wallpaper &wallpaper, const std::string &workshopRoot)
{
    if (workshopRoot.empty() || wallpaper.basePath.empty())
        return wallpaper.id;
    const std::string prefix = workshopRoot.back() == '/' ? workshopRoot : workshopRoot + "/";
    if (wallpaper.basePath.rfind(prefix, 0) == 0)
        return wallpaper.id;
    return wallpaper.basePath;
}
} // namespace

EngineCommand Renderer::render(const std::string &monitor, const Wallpaper &wallpaper,
                               const settings::SettingList &settings,
                               const options::GlobalOptions &globals, const GpuConfig &gpu,
                               const Screenshot &screenshot, const EngineOptions &options)
{
    std::vector<const settings::Setting *> all;
    for (const auto *option : globals.all())
        all.push_back(option);
    for (const auto &setting : settings.all())
        all.push_back(setting.get());
    return render(monitor, wallpaper, all, gpu, screenshot, options);
}

EngineCommand Renderer::render(const std::string &monitor, const Wallpaper &wallpaper,
                               const std::vector<const settings::Setting *> &allSettings,
                               const GpuConfig &gpu, const Screenshot &screenshot,
                               const EngineOptions &options)
{
    EngineCommand command;
    command.wallpaperId = wallpaper.id;

    command.argv = {"--screen-root", monitor, "--bg", backgroundArgument(wallpaper, options.workshopRoot)};

    if (!options.assetsDir.empty()) {
        command.argv.push_back("--assets-dir");
        command.argv.push_back(options.assetsDir);
    }
    if (!options.layer.empty()) {
        command.argv.push_back("--layer");
        command.argv.push_back(options.layer);
    }

    appendScreenshot(command.argv, monitor, screenshot);

    // 1. pre-property flags (volume, fps, boolean flags)
    // 2. the rest (scaling, clamp, then every --set-property)
    for (const bool pre : {true, false}) {
        for (const settings::Setting *setting : allSettings) {
            if (setting->isPrePropertyFlag() != pre)
                continue;
            for (const auto &arg : setting->toCommandArgs())
                command.argv.push_back(arg);
        }
    }

    appendEnvironment(command.env, gpu);
    return command;
}

} // namespace nyaa::domain
