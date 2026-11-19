#pragma once

#include <string>
#include <vector>

namespace nyaa::domain::settings {

/// One customizable property of a wallpaper.
///
/// Domain rules: no Qt, no I/O, no widget code. It answers two questions only:
/// what arguments does the engine get, and how do I persist myself.
class Setting
{
public:
    Setting(const std::string &name, const std::string &description);
    virtual ~Setting() = default;   // deleting through Setting* must be safe

    Setting(const Setting &) = delete;             // identity, not a value
    Setting &operator=(const Setting &) = delete;

    virtual std::string getName() const;
    virtual std::string getDescription() const;

    /// The argv fragment. Empty for labels and group headers.
    virtual std::vector<std::string> toCommandArgs() const = 0;
    /// What the config file stores.
    virtual std::string getValueString() const = 0;
    /// Inverse of getValueString(); bad input must leave the value unchanged.
    virtual void setValueFromString(const std::string &value) = 0;

    /// true: emitted before --set-property (volume, fps); false: after
    /// (scaling, clamp). The renderer preserves this order.
    virtual bool isPrePropertyFlag() const { return true; }

protected:
    const std::string name_;
    const std::string description_;
};

} // namespace nyaa::domain::settings
