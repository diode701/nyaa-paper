#pragma once

#include <QObject>

class ConfigBridge;

/// How grid previews animate. One value, stored in config.json as
/// preview.animation ("always" / "hover" / "off"): the G key and the Settings
/// dialog change the same thing.
class PreviewSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int animationMode READ animationMode WRITE setAnimationMode NOTIFY changed)

public:
    enum Mode { Off = 0, Hover = 1, Always = 2 };
    Q_ENUM(Mode)

    explicit PreviewSettings(ConfigBridge &config, QObject *parent = nullptr);

    int animationMode() const;
    void setAnimationMode(int mode);
    /// G: always → off → hover → always (cycles like before) and saves.
    Q_INVOKABLE void cycle();

Q_SIGNALS:
    void changed();

private:
    ConfigBridge &m_config;
};
