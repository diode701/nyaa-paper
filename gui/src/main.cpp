#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <cstdio>

#include <QKeyEvent>
#include <QKeySequence>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml>

#include "app/StartupRestore.hpp"
#include "app/Tray.hpp"
#include "bridge/ConfigBridge.hpp"
#include "bridge/EngineBridge.hpp"
#include "bridge/PlaylistBridge.hpp"
#include "bridge/PropertiesController.hpp"
#include <memory>
#include "models/DbusLibrary.hpp"
#include "models/GlobalOptionsModel.hpp"
#include "models/PropertyModel.hpp"
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

    QQuickStyle::setStyle(QStringLiteral("Material"));

    QApplication app(argc, argv);   // QApplication: the tray icon needs widgets
    // NyaaLauncher --tray: start hidden in the tray (used by autostart)
    const bool trayStart = app.arguments().contains(QStringLiteral("--tray"));
    app.setApplicationName(QStringLiteral("nyaa-paper"));
    app.setOrganizationName(QStringLiteral("diode701"));
    app.setDesktopFileName(QStringLiteral("io.github.diode701.NyaaPaper"));

    Theme theme;
    ConfigBridge config;
    PreviewSettings settings(config);
    EngineBridge engine;
    WallpaperModel library;
    PlaylistBridge playlist(library, config);
    // Monitors: detected screens + names added in Settings (Java: monitorConfig)
    const auto syncMonitors = [&engine, &playlist, &config] {
        engine.setCustomMonitors(config.customMonitors().split(QLatin1Char(','), Qt::SkipEmptyParts));
        playlist.setConnectedMonitors(engine.monitors());
    };
    syncMonitors();
    QObject::connect(&config, &ConfigBridge::saved, &app, syncMonitors);
    Tray tray(engine);
    StartupRestore restore(config, engine, library);
    PropertyModel properties;
    GlobalOptionsModel globalOptions;
    PropertiesController controller(properties, globalOptions, engine, playlist, config);
    if (qEnvironmentVariableIsSet("NYAA_MOCK")) {
        library.setAll(MockLibrary::load());
        restore.libraryLoaded();
    } else {
        auto *dbusLibrary = new DbusLibrary(&app);
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded,
                         &library, &WallpaperModel::setAll);
        // autoRestoreWallpapers runs once the library is known
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded, &restore,
                         [&restore](const QVector<WallpaperItem> &items) {
                             if (!items.isEmpty())
                                 restore.libraryLoaded();
                         });
        // Builder.build(): once at startup, drop saved configs of wallpapers
        // that are no longer subscribed.
        auto cleaned = std::make_shared<bool>(false);
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded, &controller,
                         [&controller, cleaned](const QVector<WallpaperItem> &items) {
                             if (*cleaned || items.isEmpty())
                                 return;
                             *cleaned = true;
                             QStringList ids;
                             for (const auto &item : items)
                                 ids << item.id;
                             controller.forgetUnsubscribed(ids);
                         });
        // No daemon yet? Show the sample deck rather than an empty window.
        QObject::connect(dbusLibrary, &DbusLibrary::failed, &library, [&library](const QString &message) {
            qWarning("library: %s", qUtf8Printable(message));
            if (library.count() == 0)
                library.setAll(MockLibrary::sample());
        });
    }

    qmlRegisterSingletonInstance("NyaaPaper.Theme", 1, 0, "Theme", &theme);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Settings", &settings);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Config", &config);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Playlist", &playlist);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Properties", &properties);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "GlobalOptions", &globalOptions);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Engine", &engine);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Controller", &controller);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Library", &library);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Tray", &tray);

    QQmlApplicationEngine qml;
    qml.loadFromModule("NyaaPaper", "Main");
    if (qml.rootObjects().isEmpty())
        return 1;

    if (trayStart && tray.available())
        QMetaObject::invokeMethod(qml.rootObjects().first(), "hideToTray");

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

    return app.exec();
}
