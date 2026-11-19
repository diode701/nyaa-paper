#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "domain/settings/Setting.h"

namespace nyaa::domain::options {

using settings::Kind;
using settings::Setting;

/// An engine-wide option. Java: `abstract class GlobalOption extends Setting`.
/// It is a Setting like any wallpaper property, so one list holds both and
/// one loop captures, restores and renders them (Java's allSettings).
/// What sets it apart: it has no project.json key, and its flags travel
/// before the --set-property block unless it says otherwise.
class GlobalOption : public Setting
{
public:
    using Setting::Setting;
};

class VolumeOption final : public GlobalOption
{
public:
    VolumeOption() : GlobalOption("volume", "Audio") {}
    Kind getKind() const override { return Kind::Volume; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

    int getVolume() const { return volume_; }
    void setVolume(int volume) { volume_ = volume; }
    bool isMuted() const { return muted_; }
    void setMuted(bool muted) { muted_ = muted; }

private:
    int volume_ = 70;
    bool muted_ = false;
};

class FpsOption final : public GlobalOption
{
public:
    FpsOption() : GlobalOption("fps", "Frame Rate Limit (0 = unlimited)") {}
    Kind getKind() const override { return Kind::Fps; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

    int getFps() const { return fps_; }
    void setFps(int fps) { fps_ = fps; }
    bool isLimited() const { return limited_; }
    void setLimited(bool limited) { limited_ = limited; }

private:
    int fps_ = 30;   // LWE default
    bool limited_ = false;
};

class ScalingOption final : public GlobalOption
{
public:
    ScalingOption() : GlobalOption("scaling", "Wallpaper Scaling") {}
    Kind getKind() const override { return Kind::Scaling; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;
    bool isPrePropertyFlag() const override { return false; }

    const std::string &getScaling() const { return scaling_; }
    static const std::vector<std::string> &options();

private:
    std::string scaling_ = "default";
};

class ClampOption final : public GlobalOption
{
public:
    ClampOption() : GlobalOption("clamp", "Texture Clamping") {}
    Kind getKind() const override { return Kind::Clamp; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;
    bool isPrePropertyFlag() const override { return false; }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    const std::string &getMode() const { return mode_; }
    void setMode(const std::string &mode);
    static const std::vector<std::string> &options();

private:
    bool enabled_ = false;
    std::string mode_ = "border";   // default mode when enabled
};

/// Java: BooleanFlagSetting. The flag's CLI name is its setting name.
class FlagOption final : public GlobalOption
{
public:
    FlagOption(const std::string &flag, const std::string &label) : GlobalOption(flag, label) {}
    Kind getKind() const override { return Kind::Flag; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override { return enabled_ ? "true" : "false"; }
    void setValueFromString(const std::string &value) override;

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    bool enabled_ = false;
};

/// Java: GlobalOptionsFactory.createAll(), same eleven, same order.
namespace GlobalOptionsFactory {
std::vector<std::unique_ptr<GlobalOption>> createAll();
}

/// The global options of one apply. Owns the factory's objects and gives
/// typed access to them for the view.
class GlobalOptions
{
public:
    GlobalOptions();

    /// In GlobalOptionsFactory order.
    std::vector<const GlobalOption *> all() const;
    std::vector<GlobalOption *> all();
    std::vector<std::string> arguments(bool prePropertyFlags) const;
    std::map<std::string, std::string> serialize() const;
    void restore(const std::map<std::string, std::string> &saved);
    /// Enables a flag by its CLI name (without --). Unknown names are ignored.
    void setFlag(const std::string &name, bool enabled);
    /// Back to factory defaults (Java built fresh options per properties view).
    void reset();

    VolumeOption &volume();
    FpsOption &fps();
    ScalingOption &scaling();
    ClampOption &clamp();
    std::vector<FlagOption *> flags();
    const VolumeOption &volume() const;
    const FpsOption &fps() const;
    const ScalingOption &scaling() const;
    const ClampOption &clamp() const;

private:
    template <class T> T &find() const;

    std::vector<std::unique_ptr<GlobalOption>> options_;
};

} // namespace nyaa::domain::options
