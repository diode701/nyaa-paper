#include "domain/options/GlobalOptions.h"

#include <algorithm>

namespace nyaa::domain::options {

namespace {
/// Java: Integer.parseInt, with a fallback when it fails.
int parseInt(const std::string &text, int fallback)
{
    try {
        size_t used = 0;
        const int parsed = std::stoi(text, &used);
        return used == text.size() ? parsed : fallback;
    } catch (...) {
        return fallback;
    }
}
} // namespace

// ---- VolumeSetting ---------------------------------------------------------
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
        volume_ = parseInt(value.substr(7), 70);
    } else {
        muted_ = false;
        volume_ = parseInt(value, 70);
    }
}

// ---- FpsSetting ------------------------------------------------------------
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
    fps_ = parseInt(value.substr(bar + 1), 30);
}

// ---- ScalingSetting --------------------------------------------------------
const std::vector<std::string> &ScalingOption::options()
{
    static const std::vector<std::string> values{"stretch", "fit", "fill", "default"};
    return values;
}

std::vector<std::string> ScalingOption::toCommandArgs() const
{
    if (scaling_ == "default")
        return {};
    return {"--scaling", scaling_};
}

std::string ScalingOption::getValueString() const { return scaling_; }

void ScalingOption::setValueFromString(const std::string &value)
{
    const auto &values = options();
    if (std::find(values.begin(), values.end(), value) != values.end())
        scaling_ = value;
}

// ---- ClampSetting ----------------------------------------------------------
const std::vector<std::string> &ClampOption::options()
{
    static const std::vector<std::string> values{"clamp", "border", "repeat"};
    return values;
}

void ClampOption::setMode(const std::string &mode)
{
    const auto &values = options();
    if (std::find(values.begin(), values.end(), mode) != values.end())
        mode_ = mode;
}

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
    setMode(value.substr(bar + 1));
}

// ---- BooleanFlagSetting ----------------------------------------------------
std::vector<std::string> FlagOption::toCommandArgs() const
{
    if (!enabled_)
        return {};
    return {"--" + name_};
}

void FlagOption::setValueFromString(const std::string &value)
{
    enabled_ = value == "true" || value == "TRUE" || value == "True";   // equalsIgnoreCase
}

// ---- GlobalOptionsFactory --------------------------------------------------
std::vector<std::unique_ptr<GlobalOption>> GlobalOptionsFactory::createAll()
{
    std::vector<std::unique_ptr<GlobalOption>> all;
    all.push_back(std::make_unique<VolumeOption>());
    all.push_back(std::make_unique<FpsOption>());
    all.push_back(std::make_unique<ScalingOption>());
    all.push_back(std::make_unique<ClampOption>());
    all.push_back(std::make_unique<FlagOption>("disable-mouse", "Disable Mouse Interaction"));
    all.push_back(std::make_unique<FlagOption>("disable-parallax", "Disable Parallax"));
    all.push_back(std::make_unique<FlagOption>("noautomute", "Don't Auto-mute"));
    all.push_back(std::make_unique<FlagOption>("no-audio-processing", "Disable Audio Processing"));
    all.push_back(std::make_unique<FlagOption>("no-fullscreen-pause", "Don't Pause on Fullscreen"));
    all.push_back(std::make_unique<FlagOption>("fullscreen-pause-only-active",
                                               "Pause Only When Active Fullscreen (Wayland)"));
    all.push_back(std::make_unique<FlagOption>("disable-particles", "Disable particles"));
    return all;
}

// ---- GlobalOptions ---------------------------------------------------------
GlobalOptions::GlobalOptions()
    : options_(GlobalOptionsFactory::createAll())
{
}

void GlobalOptions::reset()
{
    options_ = GlobalOptionsFactory::createAll();
}

std::vector<const GlobalOption *> GlobalOptions::all() const
{
    std::vector<const GlobalOption *> out;
    for (const auto &option : options_)
        out.push_back(option.get());
    return out;
}

std::vector<GlobalOption *> GlobalOptions::all()
{
    std::vector<GlobalOption *> out;
    for (const auto &option : options_)
        out.push_back(option.get());
    return out;
}

std::vector<std::string> GlobalOptions::arguments(bool prePropertyFlags) const
{
    std::vector<std::string> out;
    for (const auto &option : options_) {
        if (option->isPrePropertyFlag() != prePropertyFlags)
            continue;
        const auto args = option->toCommandArgs();
        out.insert(out.end(), args.begin(), args.end());
    }
    return out;
}

std::map<std::string, std::string> GlobalOptions::serialize() const
{
    std::map<std::string, std::string> out;
    for (const auto &option : options_)
        out[option->getName()] = option->getValueString();
    return out;
}

void GlobalOptions::restore(const std::map<std::string, std::string> &saved)
{
    for (const auto &option : options_) {
        const auto it = saved.find(option->getName());
        if (it != saved.end())
            option->setValueFromString(it->second);
    }
}

void GlobalOptions::setFlag(const std::string &name, bool enabled)
{
    for (FlagOption *flag : flags())
        if (flag->getName() == name)
            flag->setEnabled(enabled);
}

template <class T> T &GlobalOptions::find() const
{
    for (const auto &option : options_)
        if (auto *typed = dynamic_cast<T *>(option.get()))
            return *typed;
    std::abort();   // the factory always creates one of each
}

VolumeOption &GlobalOptions::volume() { return find<VolumeOption>(); }
FpsOption &GlobalOptions::fps() { return find<FpsOption>(); }
ScalingOption &GlobalOptions::scaling() { return find<ScalingOption>(); }
ClampOption &GlobalOptions::clamp() { return find<ClampOption>(); }
const VolumeOption &GlobalOptions::volume() const { return find<VolumeOption>(); }
const FpsOption &GlobalOptions::fps() const { return find<FpsOption>(); }
const ScalingOption &GlobalOptions::scaling() const { return find<ScalingOption>(); }
const ClampOption &GlobalOptions::clamp() const { return find<ClampOption>(); }

std::vector<FlagOption *> GlobalOptions::flags()
{
    std::vector<FlagOption *> out;
    for (const auto &option : options_)
        if (auto *flag = dynamic_cast<FlagOption *>(option.get()))
            out.push_back(flag);
    return out;
}

} // namespace nyaa::domain::options
