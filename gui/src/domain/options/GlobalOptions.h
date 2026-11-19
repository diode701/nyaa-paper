#pragma once

#include <map>
#include <string>
#include <vector>

namespace nyaa::domain::options {

/// An engine-wide flag. It has no project.json key, so it is NOT a Setting:
/// no name/description, just a value and the argv it produces.
class GlobalOption
{
public:
    virtual ~GlobalOption() = default;

    virtual std::string getName() const = 0;
    virtual std::vector<std::string> toCommandArgs() const = 0;
    virtual std::string getValueString() const = 0;
    virtual void setValueFromString(const std::string &value) = 0;

    /// true: before --set-property (volume, fps, boolean flags)
    /// false: after it (scaling, clamp)
    virtual bool isPrePropertyFlag() const { return true; }
};

class VolumeOption final : public GlobalOption
{
public:
    std::string getName() const override { return "volume"; }
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
    std::string getName() const override { return "fps"; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

    int getFps() const { return fps_; }
    void setFps(int fps) { fps_ = fps; }
    bool isLimited() const { return limited_; }
    void setLimited(bool limited) { limited_ = limited; }

private:
    int fps_ = 30;
    bool limited_ = false;
};

class ScalingOption final : public GlobalOption
{
public:
    std::string getName() const override { return "scaling"; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;
    bool isPrePropertyFlag() const override { return false; }

    const std::string &getScaling() const { return scaling_; }
    void setScaling(const std::string &scaling) { scaling_ = scaling; }

private:
    std::string scaling_ = "default";
};

class ClampOption final : public GlobalOption
{
public:
    std::string getName() const override { return "clamp"; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;
    bool isPrePropertyFlag() const override { return false; }

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    const std::string &getMode() const { return mode_; }
    void setMode(const std::string &mode) { mode_ = mode; }

private:
    bool enabled_ = false;
    std::string mode_ = "border";
};

class FlagOption final : public GlobalOption
{
public:
    explicit FlagOption(std::string flag, std::string label)
        : flag_(std::move(flag))
        , label_(std::move(label))
    {
    }

    std::string getName() const override { return flag_; }
    std::string getLabel() const { return label_; }
    std::vector<std::string> toCommandArgs() const override;
    std::string getValueString() const override { return enabled_ ? "true" : "false"; }
    void setValueFromString(const std::string &value) override;

    bool isEnabled() const { return enabled_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    std::string flag_;
    std::string label_;
    bool enabled_ = false;
};

/// All of the above, with the save/restore loops they need.
class GlobalOptions
{
public:
    GlobalOptions();

    std::vector<const GlobalOption *> all() const;
    std::vector<std::string> arguments(bool prePropertyFlags) const;
    std::map<std::string, std::string> serialize() const;
    void restore(const std::map<std::string, std::string> &saved);
    /// Enables a flag by its CLI name (without --). Unknown names are ignored.
    void setFlag(const std::string &name, bool enabled);

private:
    VolumeOption volume_;
    FpsOption fps_;
    ScalingOption scaling_;
    ClampOption clamp_;
    std::vector<FlagOption> flags_;
};

} // namespace nyaa::domain::options
