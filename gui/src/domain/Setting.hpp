#pragma once

#include <string>

#include "Kind.hpp"

/// One customizable property of a wallpaper.
///
/// Domain rules: no Qt, no I/O, no widget code. It answers two questions only —
/// "what argv fragment do I produce" and "how do I persist myself".
class Setting
{
public:
    Setting(std::string key, std::string label)
        : m_key(std::move(key))
        , m_label(std::move(label))
    {
    }

    virtual ~Setting() = default;          // deleting through Setting* must be safe

    Setting(const Setting &) = delete;      // identity, not a value
    Setting &operator=(const Setting &) = delete;
    Setting(Setting &&) = default;
    Setting &operator=(Setting &&) = default;

    const std::string &key() const { return m_key; }
    const std::string &label() const { return m_label; }

    virtual Kind kind() const = 0;
    /// "--set-property key=value", or empty for text/group which emit nothing.
    virtual std::string argument() const = 0;
    /// What the config file stores.
    virtual std::string serialize() const = 0;
    /// Inverse of serialize(); bad input must leave the value unchanged.
    virtual void restore(const std::string &value) = 0;
    /// What the UI shows next to the control.
    virtual std::string display() const = 0;

protected:
    std::string m_key;
    std::string m_label;
};
