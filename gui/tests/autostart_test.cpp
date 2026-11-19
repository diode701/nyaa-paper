// Autostart files (TODO J5): written when enabled, removed when disabled.
#include <cassert>
#include <cstdio>

#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>

#include "app/Autostart.hpp"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir dir;
    const Autostart::Paths paths{dir.filePath("config"), dir.filePath("data")};

    assert(Autostart::apply(true, paths, "/opt/nyaa/nyaa-paper-qt", "/opt/nyaa/nyaa-paperd"));
    QFile desktop(Autostart::desktopFile(paths));
    assert(desktop.open(QIODevice::ReadOnly));
    const QByteArray entry = desktop.readAll();
    assert(entry.contains("Exec=\"/opt/nyaa/nyaa-paper-qt\" --tray"));
    assert(entry.contains("X-GNOME-Autostart-enabled=true"));
    QFile service(Autostart::busServiceFile(paths));
    assert(service.open(QIODevice::ReadOnly));
    const QByteArray bus = service.readAll();
    assert(bus.contains("Name=org.nyaa.Paper") && bus.contains("Exec=/opt/nyaa/nyaa-paperd"));

    // no daemon path known: the login entry alone
    assert(Autostart::apply(false, paths, {}, {}));
    assert(!QFile::exists(Autostart::desktopFile(paths)));
    assert(!QFile::exists(Autostart::busServiceFile(paths)));
    assert(Autostart::apply(true, paths, "/x/gui", {}));
    assert(QFile::exists(Autostart::desktopFile(paths)));
    assert(!QFile::exists(Autostart::busServiceFile(paths)));

    // disabling twice is fine
    assert(Autostart::apply(false, paths, {}, {}));
    assert(Autostart::apply(false, paths, {}, {}));

    std::puts("autostart tests passed");
    return 0;
}
