#include "BoolSetting.hpp"

#include <utility>

BoolSetting::BoolSetting(std::string key, std::string label, bool value)
    : CustomSetting<bool>(std::move(key), std::move(label), value)
{
}

Kind BoolSetting::kind() const { return Kind::Bool; }

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
