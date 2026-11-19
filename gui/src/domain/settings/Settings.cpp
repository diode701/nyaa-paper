#include "domain/settings/Settings.h"

#include <cstdio>
#include <format>
#include <utility>

namespace nyaa::domain::settings {

namespace {
double clamp01(double v)
{
    return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}
} // namespace

// ---- bool ------------------------------------------------------------------
std::string BooleanSetting::valueToString() const { return value_ ? "1" : "0"; }

std::string BooleanSetting::getValueString() const { return valueToString(); }

void BooleanSetting::setValueFromString(const std::string &value)
{
    if (value == "1" || value == "true" || value == "True")
        value_ = true;
    else if (value == "0" || value == "false" || value == "False")
        value_ = false;
    // anything else keeps the current value on purpose
}

// ---- slider ----------------------------------------------------------------
SliderSetting::SliderSetting(const std::string &name, const std::string &description, double value,
                             SliderRange range)
    : CustomSetting<double>(name, description, value)
    , range_(range)
{
}

std::string SliderSetting::valueToString() const { return std::format("{:.2f}", value_); }

std::string SliderSetting::getValueString() const { return valueToString(); }

void SliderSetting::setValueFromString(const std::string &value)
{
    try {
        value_ = std::stod(value);
    } catch (...) {
        // keep the current value
    }
}

// ---- color -----------------------------------------------------------------
std::string ColorSetting::valueToString() const
{
    return std::format("{:.4f} {:.4f} {:.4f}", value_.r, value_.g, value_.b);
}

std::string ColorSetting::getValueString() const { return valueToString(); }

void ColorSetting::setValueFromString(const std::string &value)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (std::sscanf(value.c_str(), "%lf %lf %lf", &r, &g, &b) == 3)
        value_ = {clamp01(r), clamp01(g), clamp01(b)};
}

std::string ColorSetting::getHex() const
{
    const int r = static_cast<int>(clamp01(value_.r) * 255.0 + 0.5);
    const int g = static_cast<int>(clamp01(value_.g) * 255.0 + 0.5);
    const int b = static_cast<int>(clamp01(value_.b) * 255.0 + 0.5);
    return std::format("#{:02X}{:02X}{:02X}", r, g, b);
}

// ---- combo -----------------------------------------------------------------
ComboSetting::ComboSetting(const std::string &name, const std::string &description, int value,
                           std::vector<std::pair<int, std::string>> options)
    : CustomSetting<int>(name, description, value)
    , options_(std::move(options))
{
}

std::string ComboSetting::valueToString() const { return std::to_string(value_); }

std::string ComboSetting::getValueString() const { return valueToString(); }

void ComboSetting::setValueFromString(const std::string &value)
{
    try {
        value_ = std::stoi(value);
    } catch (...) {
        // keep the current value
    }
}

std::string ComboSetting::getOptionLabel() const
{
    for (const auto &option : options_) {
        if (option.first == value_)
            return option.second;
    }
    return std::to_string(value_);
}

} // namespace nyaa::domain::settings
