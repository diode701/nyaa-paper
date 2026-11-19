#pragma once

#include <QObject>
#include <QSettings>

/// User preferences for the client. Stored at ~/.config/nyaa-paper/gui.ini so
/// dev runs and the flatpak share one file.
class PreviewSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int animationMode READ animationMode WRITE setAnimationMode NOTIFY changed)

public:
    enum Mode { Off = 0, Hover = 1, Always = 2 };
    Q_ENUM(Mode)

    explicit PreviewSettings(QObject *parent = nullptr);

    int animationMode() const { return m_mode; }
    void setAnimationMode(int mode);
    Q_INVOKABLE void cycle();

Q_SIGNALS:
    void changed();

private:
    QSettings m_store;
    int m_mode = Always;
};
