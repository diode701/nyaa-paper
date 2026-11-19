#pragma once

#include "CustomSetting.hpp"

class ColorSetting final : public CustomSetting<Rgb>
{
public:
    ColorSetting(std::string key, std::string label, Rgb value);

    Kind kind() const override;
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
};
