#pragma once

#include <utility>

#include "Setting.hpp"

/// A Setting that owns one typed value. A template, so it lives in a header:
/// the compiler needs the body to instantiate it.
template <class T>
class CustomSetting : public Setting
{
public:
    T value() const { return m_value; }
    void setValue(T value) { m_value = std::move(value); }

protected:
    CustomSetting(std::string key, std::string label, T value)
        : Setting(std::move(key), std::move(label))
        , m_value(std::move(value))
    {
    }

    T m_value;
};
