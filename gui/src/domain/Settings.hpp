#pragma once

#include "domain/CustomSetting.hpp"

namespace nyaa::domain {

// The ten concrete property kinds. They share one translation unit because they
// share one job: turn a value into an argv fragment. The only way to create one
// is SettingFactory, so nothing else needs their headers.

class BoolSetting final : public CustomSetting<bool>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::Bool; }
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
};

class SliderSetting final : public CustomSetting<double>
{
public:
    SliderSetting(std::string key, std::string label, double value, SliderRange range);
    Kind kind() const override { return Kind::Slider; }
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
    const SliderRange &range() const { return m_range; }

private:
    SliderRange m_range;
};

class ColorSetting final : public CustomSetting<Rgb>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::Color; }
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
};

class ComboSetting final : public CustomSetting<int>
{
public:
    ComboSetting(std::string key, std::string label, int value,
                 std::vector<std::pair<int, std::string>> options);
    Kind kind() const override { return Kind::Combo; }
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
    const std::vector<std::pair<int, std::string>> &options() const { return m_options; }

private:
    std::vector<std::pair<int, std::string>> m_options;
};

class TextSetting final : public Setting
{
public:
    using Setting::Setting;
    Kind kind() const override { return Kind::Text; }
    std::string argument() const override { return {}; }
    std::string serialize() const override { return {}; }
    void restore(const std::string &) override {}
    std::string display() const override { return {}; }
    bool emitsArgument() const override { return false; }
};

class GroupSetting final : public Setting
{
public:
    using Setting::Setting;
    Kind kind() const override { return Kind::Group; }
    std::string argument() const override { return {}; }
    std::string serialize() const override { return {}; }
    void restore(const std::string &) override {}
    std::string display() const override { return {}; }
    bool emitsArgument() const override { return false; }
};

class TextInputSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::TextInput; }
    std::string argument() const override;
    std::string serialize() const override { return m_value; }
    void restore(const std::string &value) override;
    std::string display() const override { return m_value; }
};

class FileSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::File; }
    std::string argument() const override;
    std::string serialize() const override { return m_value; }
    void restore(const std::string &value) override;
    std::string display() const override { return m_value; }
};

class DirectorySetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::Directory; }
    std::string argument() const override;
    std::string serialize() const override { return m_value; }
    void restore(const std::string &value) override;
    std::string display() const override { return m_value; }
};

class TextureSetting final : public CustomSetting<std::string>
{
public:
    using CustomSetting::CustomSetting;
    Kind kind() const override { return Kind::Texture; }
    std::string argument() const override;
    std::string serialize() const override { return m_value; }
    void restore(const std::string &value) override;
    std::string display() const override { return m_value; }
};

} // namespace nyaa::domain
