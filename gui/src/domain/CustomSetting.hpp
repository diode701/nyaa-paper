#pragma once

#include <utility>

#include "domain/Setting.hpp"

namespace nyaa::domain {

/// A Setting that owns one typed value. A template, so it lives in a header:
/// the compiler needs the body to instantiate it.
template <class T>
class CustomSetting : public Setting
{
public:
    T value() const { return m_value; }
    void setValue(T value) { m_value = std::move(value); }

    // public so `using CustomSetting::CustomSetting;` in a leaf inherits it as a
    // public constructor: inherited constructors keep the base's access level.
    CustomSetting(std::string key, std::string label, T value)
        : Setting(std::move(key), std::move(label))
        , m_value(std::move(value))
    {
    }

protected:
    T m_value;
};

} // namespace nyaa::domain
