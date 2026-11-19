#include "ColorSetting.hpp"

#include <cstdio>
#include <format>
#include <utility>

namespace {
double clamp01(double v) { return v < 0.0 ? 0.0 : (v > 1.0 ? 1.0 : v); }
}

ColorSetting::ColorSetting(std::string key, std::string label, Rgb value)
    : CustomSetting<Rgb>(std::move(key), std::move(label), value)
{
}

Kind ColorSetting::kind() const { return Kind::Color; }

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
