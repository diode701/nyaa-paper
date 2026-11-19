#pragma once

#include "domain/settings/CustomSetting.h"
#include "domain/settings/Kind.h"

namespace nyaa::domain::settings {

// The ten property kinds. They share one translation unit because they share one
// job: turn a value into an argv fragment and back.

class BooleanSetting final : public CustomSetting<bool>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

protected:
    std::string valueToString() const override;
};

class SliderSetting final : public CustomSetting<double>
{
public:
    SliderSetting(const std::string &name, const std::string &description, double value, SliderRange range);

    const SliderRange &getRange() const { return range_; }
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

protected:
    std::string valueToString() const override;

private:
    SliderRange range_;
};

class ColorSetting final : public CustomSetting<Rgb>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;
    /// "#RRGGBB" for the UI.
    std::string getHex() const;

protected:
    std::string valueToString() const override;
};

class ComboSetting final : public CustomSetting<int>
{
public:
    ComboSetting(const std::string &name, const std::string &description, int value,
                 std::vector<std::pair<int, std::string>> options);

    const std::vector<std::pair<int, std::string>> &getOptions() const { return options_; }
    /// The label for the current value; falls back to the number.
    std::string getOptionLabel() const;
    std::string getValueString() const override;
    void setValueFromString(const std::string &value) override;

protected:
    std::string valueToString() const override;

private:
    std::vector<std::pair<int, std::string>> options_;
};

class TextSetting final : public Setting
{
public:
    using Setting::Setting;
    std::vector<std::string> toCommandArgs() const override { return {}; }
    std::string getValueString() const override { return {}; }
    void setValueFromString(const std::string &) override {}
};

class GroupSetting final : public Setting
{
public:
    using Setting::Setting;
    std::vector<std::string> toCommandArgs() const override { return {}; }
    std::string getValueString() const override { return {}; }
    void setValueFromString(const std::string &) override {}
};

class TextInputSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override { return value_; }
    void setValueFromString(const std::string &value) override { value_ = value; }

protected:
    std::string valueToString() const override { return value_; }
};

class FileSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override { return value_; }
    void setValueFromString(const std::string &value) override { value_ = value; }

protected:
    std::string valueToString() const override { return value_; }
};

class DirectorySetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override { return value_; }
    void setValueFromString(const std::string &value) override { value_ = value; }

protected:
    std::string valueToString() const override { return value_; }
};

class TextureSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    std::string getValueString() const override { return value_; }
    void setValueFromString(const std::string &value) override { value_ = value; }

protected:
    std::string valueToString() const override { return value_; }
};

} // namespace nyaa::domain::settings
