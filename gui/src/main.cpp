#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <QtQml>

#include "app/DevHooks.hpp"
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
    PropertiesController controller(properties, globalOptions, engine, playlist, config, library);
    if (qEnvironmentVariableIsSet("NYAA_MOCK")) {
        library.setAll(MockLibrary::load());
        restore.libraryLoaded();
    } else {
        auto *dbusLibrary = new DbusLibrary(&app);
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded,
                         &library, &WallpaperModel::setAll);
        // autoRestoreWallpapers runs once the library is known
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded, &restore,
                         [&restore](const QVector<nyaa::domain::Wallpaper> &items) {
                             if (!items.isEmpty())
                                 restore.libraryLoaded();
                         });
        // Builder.build(): once at startup, drop saved configs of wallpapers
        // that are no longer subscribed.
        auto cleaned = std::make_shared<bool>(false);
        QObject::connect(dbusLibrary, &DbusLibrary::wallpapersLoaded, &controller,
                         [&controller, cleaned](const QVector<nyaa::domain::Wallpaper> &items) {
                             if (*cleaned || items.isEmpty())
                                 return;
                             *cleaned = true;
                             QStringList ids;
                             for (const auto &item : items)
                                 ids << QString::fromStdString(item.id);
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

    // NYAA_WINDOW / NYAA_KEYS / NYAA_SHOT / ... for screenshots and smoke tests
    installDevHooks(app, qml);

    return app.exec();
}
