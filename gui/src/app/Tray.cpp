#include "Tray.hpp"

#include <QApplication>
#include <QMenu>
#include <QSystemTrayIcon>

#include "bridge/EngineBridge.hpp"

Tray::Tray(EngineBridge &engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_icon = new QSystemTrayIcon(QIcon(QStringLiteral(":/icon-128.png")), this);
    m_icon->setToolTip(QStringLiteral("Nyaa Paper"));

    auto *menu = new QMenu();
    connect(this, &QObject::destroyed, menu, &QObject::deleteLater);
    menu->addAction(QStringLiteral("Restore"), this, &Tray::restoreRequested);
    menu->addSeparator();
    // The wallpapers live in the daemon and keep running after the window
    // quits; Java's Exit stopped them, which is the second entry.
    menu->addAction(QStringLiteral("Quit"), qApp, &QCoreApplication::quit);
    menu->addAction(QStringLiteral("Quit and stop wallpapers"), this, [this] {
        m_engine.stopAll();
        QCoreApplication::quit();
    });
    m_icon->setContextMenu(menu);

    connect(m_icon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
            emit restoreRequested();
    });
    m_icon->show();
}
