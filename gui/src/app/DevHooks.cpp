#include "app/DevHooks.hpp"

#include <cstdio>

#include <QApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

void installDevHooks(QApplication &app, QQmlApplicationEngine &qml)
{
    // Dev aid: NYAA_WINDOW=2540x1422 renders at the user's real window size.
    if (qEnvironmentVariableIsSet("NYAA_WINDOW")) {
        const QString size = qEnvironmentVariable("NYAA_WINDOW");
        const QStringList parts = size.split(QLatin1Char('x'));
        if (parts.size() == 2) {
            qml.rootObjects().first()->setProperty("width", parts[0].toInt());
            qml.rootObjects().first()->setProperty("height", parts[1].toInt());
        }
    }

    // Dev aid: NYAA_GLOBAL=1 opens the properties view on the GLOBAL tab.
    if (qEnvironmentVariableIsSet("NYAA_MINI"))
        qml.rootObjects().first()->setProperty("startMini", true);

    if (qEnvironmentVariableIsSet("NYAA_GLOBAL"))
        qml.rootObjects().first()->setProperty("startGlobal", true);

    // Dev aid: NYAA_KEYS="Right,Right,Down" injects keys after startup.
    if (qEnvironmentVariableIsSet("NYAA_KEYS")) {
        const QStringList keys = qEnvironmentVariable("NYAA_KEYS").split(QLatin1Char(','));
        QTimer::singleShot(250, &app, [&qml] {
            if (auto *window = qobject_cast<QWindow *>(qml.rootObjects().value(0))) {
                window->requestActivate();
                window->setKeyboardGrabEnabled(true);
            }
        });
        QTimer::singleShot(700, &app, [&app, &qml, keys] {
            auto *window = qobject_cast<QWindow *>(qml.rootObjects().value(0));
            if (!window)
                return;
            for (const QString &name : keys) {
                const QKeySequence seq(name);
                if (seq.isEmpty())
                    continue;
                QKeyEvent press(QEvent::KeyPress, seq[0].key(), Qt::NoModifier);
                QCoreApplication::sendEvent(window, &press);
            }
            QTimer::singleShot(120, &app, [&app, &qml, window] {
                auto *root = qml.rootObjects().value(0);
                auto *quick = qobject_cast<QQuickWindow *>(window);
                auto *focus = quick ? quick->activeFocusItem() : nullptr;
                std::fprintf(stderr, "focus=%s cursor=%d page=%d\n",
                             focus ? focus->metaObject()->className() : "none",
                             root->property("debugCursor").toInt(),
                             root->property("debugPage").toInt());
                if (!qEnvironmentVariableIsSet("NYAA_SHOT"))
                    QCoreApplication::quit();
            });
        });
    }

    // Dev aid: NYAA_VIEW=1|2 opens properties or playlist for screenshots.
    if (qEnvironmentVariableIsSet("NYAA_VIEW")) {
        const int requested = qEnvironmentVariableIntValue("NYAA_VIEW");
        if (requested == 3)
            qml.rootObjects().first()->setProperty("settingsOpen", true);
        else
            qml.rootObjects().first()->setProperty("view", requested);
    }

    // Dev aid: NYAA_SHOT=/path/frame.png renders one frame and exits.
    if (qEnvironmentVariableIsSet("NYAA_SHOT")) {
        const QString path = qEnvironmentVariable("NYAA_SHOT");
        const int delay = qEnvironmentVariableIntValue("NYAA_SHOT_DELAY");
        QTimer::singleShot(delay > 0 ? delay : 1200, &app, [&qml, path] {
            if (auto *window = qobject_cast<QQuickWindow *>(qml.rootObjects().value(0)))
                window->grabWindow().save(path);
            QCoreApplication::quit();
        });
    }
}
