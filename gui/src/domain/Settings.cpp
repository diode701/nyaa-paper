#include "domain/Settings.hpp"

#include <cstdio>
#include <format>
#include <utility>

namespace nyaa::domain {

namespace {
double clamp01(double v)
{
    return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v);
}
} // namespace

// ---- bool ------------------------------------------------------------------
std::string BoolSetting::argument() const
{
    return "--set-property " + m_key + "=" + (m_value ? "1" : "0");
}

std::string BoolSetting::serialize() const { return m_value ? "1" : "0"; }

void BoolSetting::restore(const std::string &value)
{
    if (value == "1" || value == "true" || value == "True")
        m_value = true;
    else if (value == "0" || value == "false" || value == "False")
        m_value = false;
    // anything else keeps the current value on purpose
}

std::string BoolSetting::display() const { return m_value ? "on" : "off"; }

// ---- slider ----------------------------------------------------------------
SliderSetting::SliderSetting(std::string key, std::string label, double value, SliderRange range)
    : CustomSetting<double>(std::move(key), std::move(label), value)
    , m_range(range)
{
}

std::string SliderSetting::argument() const
{
    return std::format("--set-property {}={:.2f}", m_key, m_value);
}

std::string SliderSetting::serialize() const { return std::format("{:.2f}", m_value); }

void SliderSetting::restore(const std::string &value)
{
    try {
        m_value = std::stod(value);
    } catch (...) {
        // keep the current value
    }
}

std::string SliderSetting::display() const { return std::format("{:.2f}", m_value); }

// ---- color -----------------------------------------------------------------
std::string ColorSetting::argument() const
{
    return std::format("--set-property {}={:.4f} {:.4f} {:.4f}", m_key, m_value.r, m_value.g, m_value.b);
}

std::string ColorSetting::serialize() const
{
    return std::format("{} {} {}", m_value.r, m_value.g, m_value.b);
}

void ColorSetting::restore(const std::string &value)
{
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    if (std::sscanf(value.c_str(), "%lf %lf %lf", &r, &g, &b) == 3)
        m_value = {clamp01(r), clamp01(g), clamp01(b)};
}

std::string ColorSetting::display() const
{
    const int r = static_cast<int>(clamp01(m_value.r) * 255.0 + 0.5);
    const int g = static_cast<int>(clamp01(m_value.g) * 255.0 + 0.5);
    const int b = static_cast<int>(clamp01(m_value.b) * 255.0 + 0.5);
    return std::format("#{:02X}{:02X}{:02X}", r, g, b);
}

// ---- combo -----------------------------------------------------------------
ComboSetting::ComboSetting(std::string key, std::string label, int value,
                           std::vector<std::pair<int, std::string>> options)
    : CustomSetting<int>(std::move(key), std::move(label), value)
    , m_options(std::move(options))
{
}

std::string ComboSetting::argument() const
{
    return std::format("--set-property {}={}", m_key, m_value);
}

std::string ComboSetting::serialize() const { return std::to_string(m_value); }

void ComboSetting::restore(const std::string &value)
{
    try {
        m_value = std::stoi(value);
    } catch (...) {
        // keep the current value
    }
}

std::string ComboSetting::display() const
{
    for (const auto &option : m_options) {
        if (option.first == m_value)
            return option.second;
    }
    return std::to_string(m_value);
}

// ---- text input ------------------------------------------------------------
std::string TextInputSetting::argument() const
{
    return "--set-property " + m_key + "=" + m_value;
}

void TextInputSetting::restore(const std::string &value) { m_value = value; }

// ---- paths -----------------------------------------------------------------
std::string FileSetting::argument() const
{
    return "--set-property " + m_key + "=" + m_value;
}

void FileSetting::restore(const std::string &value) { m_value = value; }

std::string DirectorySetting::argument() const
{
    return "--set-property " + m_key + "=" + m_value;
}

void DirectorySetting::restore(const std::string &value) { m_value = value; }

std::string TextureSetting::argument() const
{
    return "--set-property " + m_key + "=" + m_value;
}

void TextureSetting::restore(const std::string &value) { m_value = value; }

} // namespace nyaa::domain
