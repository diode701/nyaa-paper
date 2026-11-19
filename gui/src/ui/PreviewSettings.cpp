#include "PreviewSettings.hpp"

#include "bridge/ConfigBridge.hpp"

namespace {
QString nameOf(int mode)
{
    switch (mode) {
    case PreviewSettings::Off: return QStringLiteral("off");
    case PreviewSettings::Hover: return QStringLiteral("hover");
    default: return QStringLiteral("always");
    }
}
} // namespace

PreviewSettings::PreviewSettings(ConfigBridge &config, QObject *parent)
    : QObject(parent)
    , m_config(config)
{
    // The Settings dialog edits Config.animation directly; follow it live.
    connect(&m_config, &ConfigBridge::changed, this, &PreviewSettings::changed);
}

int PreviewSettings::animationMode() const
{
    const QString name = m_config.animation();
    if (name == QStringLiteral("off"))
        return Off;
    if (name == QStringLiteral("hover"))
        return Hover;
    return Always;
}

void PreviewSettings::setAnimationMode(int mode)
{
    if (mode < Off || mode > Always || mode == animationMode())
        return;
    m_config.setAnimation(nameOf(mode));
    m_config.save();
}

void PreviewSettings::cycle()
{
    setAnimationMode((animationMode() + 1) % 3);
}
