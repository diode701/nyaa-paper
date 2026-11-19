#pragma once

#include <QObject>

class EngineBridge;
class QSystemTrayIcon;

/// WallpaperBrowser's system tray: closing the window hides it here, and the
/// menu brings it back or quits. When the desktop has no tray, closing quits
/// (Java hid the window anyway and left no way back).
class Tray : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available CONSTANT)

public:
    explicit Tray(EngineBridge &engine, QObject *parent = nullptr);

    bool available() const { return m_icon != nullptr; }

Q_SIGNALS:
    /// Tray "Restore" or a click on the icon.
    void restoreRequested();

private:
    EngineBridge &m_engine;
    QSystemTrayIcon *m_icon = nullptr;
};
