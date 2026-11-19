#pragma once

#include "CustomSetting.hpp"

class SliderSetting final : public CustomSetting<double>
{
public:
    SliderSetting(std::string key, std::string label, double value, SliderRange range);

    Kind kind() const override;
    std::string argument() const override;
    std::string serialize() const override;
    void restore(const std::string &value) override;
    std::string display() const override;

    const SliderRange &range() const { return m_range; }

private:
    SliderRange m_range;
};
