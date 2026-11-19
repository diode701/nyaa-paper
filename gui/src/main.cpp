#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <cstdio>

#include <QKeyEvent>
#include <QKeySequence>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml>

#include "bridge/EngineBridge.hpp"
#include "models/DbusLibrary.hpp"
#include "models/MockLibrary.hpp"
#include "models/WallpaperModel.hpp"
#include "ui/PreviewSettings.hpp"
#include "ui/Theme.hpp"

int main(int argc, char *argv[])
{
    // Vulkan by default: it drives the shader effects (card shadows, selection
    // glow) more reliably than the GL path on NVIDIA/Wayland. Override with
    // QSG_RHI_BACKEND=opengl (or =software) when diagnosing.
    if (qEnvironmentVariableIsEmpty("QSG_RHI_BACKEND"))
        qputenv("QSG_RHI_BACKEND", "vulkan");

    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("nyaa-paper"));
    app.setOrganizationName(QStringLiteral("diode701"));
    app.setDesktopFileName(QStringLiteral("io.github.diode701.NyaaPaper"));

    Theme theme;
    PreviewSettings settings;
    EngineBridge engine;
    WallpaperModel library;
    if (qEnvironmentVariableIsSet("NYAA_MOCK")) {
        library.setAll(MockLibrary::load());
    } else {
        auto *dbusLibrary = new DbusLibrary(&app);
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded,
                         &library, &WallpaperModel::setAll);
        // No daemon yet? Show the sample deck rather than an empty window.
        QObject::connect(dbusLibrary, &DbusLibrary::failed, &library, [&library](const QString &message) {
            qWarning("library: %s", qUtf8Printable(message));
            if (library.count() == 0)
                library.setAll(MockLibrary::sample());
        });
    }

    qmlRegisterSingletonInstance("NyaaPaper.Theme", 1, 0, "Theme", &theme);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Settings", &settings);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Engine", &engine);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Library", &library);

    QQmlApplicationEngine qml;
    qml.loadFromModule("NyaaPaper", "Main");
    if (qml.rootObjects().isEmpty())
        return 1;

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
    if (qEnvironmentVariableIsSet("NYAA_VIEW"))
        qml.rootObjects().first()->setProperty("view", qEnvironmentVariableIntValue("NYAA_VIEW"));

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

    return app.exec();
}
