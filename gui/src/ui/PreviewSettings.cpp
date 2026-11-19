#include "PreviewSettings.hpp"

#include <QDir>

namespace {
QString configFile()
{
    const QString dir = QDir::homePath() + QStringLiteral("/.config/nyaa-paper");
    QDir().mkpath(dir);
    return dir + QStringLiteral("/gui.ini");
}
} // namespace

PreviewSettings::PreviewSettings(QObject *parent)
    : QObject(parent)
    , m_store(configFile(), QSettings::IniFormat)
{
    m_mode = m_store.value(QStringLiteral("previews/animation"), int(Always)).toInt();
}

void PreviewSettings::setAnimationMode(int mode)
{
    if (mode < Off || mode > Always || mode == m_mode)
        return;
    m_mode = mode;
    m_store.setValue(QStringLiteral("previews/animation"), m_mode);
    emit changed();
}

void PreviewSettings::cycle()
{
    setAnimationMode((m_mode + 1) % 3);
}
