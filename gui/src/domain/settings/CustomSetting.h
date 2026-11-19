#pragma once

#include <utility>

#include "domain/settings/Setting.h"

namespace nyaa::domain::settings {

/// A Setting that owns one typed value. A template, so the body lives in a
/// header: the compiler needs it to instantiate the class.
template <class T>
class CustomSetting : public Setting
{
public:
    CustomSetting(const std::string &name, const std::string &description, T value)
        : Setting(name, description)
        , value_(std::move(value))
    {
    }

    T value() const { return value_; }
    void setValue(T value) { value_ = std::move(value); }

    /// Every CustomSetting carries "--set-property name=value", so the pair is
    /// built here once instead of ten times in the leaves.
    std::vector<std::string> toCommandArgs() const override
    {
        return {"--set-property", name_ + "=" + valueToString()};
    }

protected:
    virtual std::string valueToString() const = 0;

    T value_;
};

} // namespace nyaa::domain::settings
