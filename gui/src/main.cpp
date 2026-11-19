#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QtQml>

#include "bridge/EngineBridge.hpp"
#include "models/MockLibrary.hpp"
#include "models/WallpaperModel.hpp"
#include "ui/PreviewSettings.hpp"
#include "ui/Theme.hpp"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("nyaa-paper"));
    app.setOrganizationName(QStringLiteral("diode701"));
    app.setDesktopFileName(QStringLiteral("io.github.diode701.NyaaPaper"));

    Theme theme;
    PreviewSettings settings;
    EngineBridge engine;
    WallpaperModel library;
    library.setAll(MockLibrary::load());

    qmlRegisterSingletonInstance("NyaaPaper.Theme", 1, 0, "Theme", &theme);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Settings", &settings);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Engine", &engine);
    qmlRegisterSingletonInstance("NyaaPaper.Backend", 1, 0, "Library", &library);

    QQmlApplicationEngine qml;
    qml.loadFromModule("NyaaPaper", "Main");
    if (qml.rootObjects().isEmpty())
        return 1;

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
