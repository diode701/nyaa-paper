#include "SliderSetting.hpp"

#include <format>
#include <utility>

SliderSetting::SliderSetting(std::string key, std::string label, double value, SliderRange range)
    : CustomSetting<double>(std::move(key), std::move(label), value)
    , m_range(range)
{
}

Kind SliderSetting::kind() const { return Kind::Slider; }

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
