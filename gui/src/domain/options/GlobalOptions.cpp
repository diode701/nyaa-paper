#include "domain/options/GlobalOptions.h"

#include <cstdlib>
#include <utility>

namespace nyaa::domain::options {

// ---- volume ----------------------------------------------------------------
std::vector<std::string> VolumeOption::toCommandArgs() const
{
    if (muted_)
        return {"--silent"};
    return {"--volume", std::to_string(volume_)};
}

std::string VolumeOption::getValueString() const
{
    return muted_ ? "silent|" + std::to_string(volume_) : std::to_string(volume_);
}

void VolumeOption::setValueFromString(const std::string &value)
{
    if (value.rfind("silent|", 0) == 0) {
        muted_ = true;
        volume_ = std::atoi(value.c_str() + 7);
    } else {
        muted_ = false;
        volume_ = std::atoi(value.c_str());
    }
}

// ---- fps -------------------------------------------------------------------
std::vector<std::string> FpsOption::toCommandArgs() const
{
    if (limited_ && fps_ > 0)
        return {"--fps", std::to_string(fps_)};
    return {};
}

std::string FpsOption::getValueString() const
{
    return (limited_ ? "true|" : "false|") + std::to_string(fps_);
}

void FpsOption::setValueFromString(const std::string &value)
{
    const auto bar = value.find('|');
    if (bar == std::string::npos)
        return;
    limited_ = value.substr(0, bar) == "true";
    fps_ = std::atoi(value.c_str() + bar + 1);
}

// ---- scaling ---------------------------------------------------------------
std::vector<std::string> ScalingOption::toCommandArgs() const
{
    if (scaling_ == "default" || scaling_.empty())
        return {};
    return {"--scaling", scaling_};
}

std::string ScalingOption::getValueString() const { return scaling_; }

void ScalingOption::setValueFromString(const std::string &value)
{
    if (value == "stretch" || value == "fit" || value == "fill" || value == "default")
        scaling_ = value;
}

// ---- clamp -----------------------------------------------------------------
std::vector<std::string> ClampOption::toCommandArgs() const
{
    if (!enabled_ || mode_.empty())
        return {};
    return {"--clamp", mode_};
}

std::string ClampOption::getValueString() const
{
    return (enabled_ ? "true|" : "false|") + mode_;
}

void ClampOption::setValueFromString(const std::string &value)
{
    const auto bar = value.find('|');
    if (bar == std::string::npos)
        return;
    enabled_ = value.substr(0, bar) == "true";
    const std::string mode = value.substr(bar + 1);
    if (mode == "clamp" || mode == "border" || mode == "repeat")
        mode_ = mode;
}

// ---- flags -----------------------------------------------------------------
std::vector<std::string> FlagOption::toCommandArgs() const
{
    if (!enabled_)
        return {};
    return {"--" + flag_};
}

void FlagOption::setValueFromString(const std::string &value)
{
    enabled_ = value == "true" || value == "1";
}

// ---- aggregate -------------------------------------------------------------
GlobalOptions::GlobalOptions()
    : flags_{FlagOption("disable-mouse", "Disable mouse interaction"),
             FlagOption("disable-parallax", "Disable parallax"),
             FlagOption("noautomute", "Don't auto-mute"),
             FlagOption("no-audio-processing", "Disable audio processing"),
             FlagOption("no-fullscreen-pause", "Don't pause on fullscreen"),
             FlagOption("fullscreen-pause-only-active", "Pause only when active fullscreen"),
             FlagOption("disable-particles", "Disable particles")}
{
}

std::vector<const GlobalOption *> GlobalOptions::all() const
{
    std::vector<const GlobalOption *> out{&volume_, &fps_, &scaling_, &clamp_};
    for (const auto &flag : flags_)
        out.push_back(&flag);
    return out;
}

std::vector<std::string> GlobalOptions::arguments(bool prePropertyFlags) const
{
    std::vector<std::string> out;
    for (const GlobalOption *option : all()) {
        if (option->isPrePropertyFlag() != prePropertyFlags)
            continue;
        const std::vector<std::string> args = option->toCommandArgs();
        out.insert(out.end(), args.begin(), args.end());
    }
    return out;
}

std::map<std::string, std::string> GlobalOptions::serialize() const
{
    std::map<std::string, std::string> out;
    for (const GlobalOption *option : all())
        out[option->getName()] = option->getValueString();
    return out;
}

void GlobalOptions::restore(const std::map<std::string, std::string> &saved)
{
    std::vector<GlobalOption *> mutableOptions{&volume_, &fps_, &scaling_, &clamp_};
    for (auto &flag : flags_)
        mutableOptions.push_back(&flag);

    for (GlobalOption *option : mutableOptions) {
        const auto it = saved.find(option->getName());
        if (it != saved.end())
            option->setValueFromString(it->second);
    }
}

} // namespace nyaa::domain::options
