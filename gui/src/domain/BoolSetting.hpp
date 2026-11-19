#pragma once

#include "CustomSetting.hpp"

class BoolSetting final : public CustomSetting<bool>
{
public:
    BoolSetting(std::string key, std::string label, bool value);

    Kind kind() const override;
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;
};
