#include "Autostart.hpp"

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>

namespace Autostart {

namespace {
bool writeFile(const QString &path, const QByteArray &content, QString *error)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(content) != content.size() || !file.commit()) {
        if (error)
            *error = QStringLiteral("cannot write %1").arg(path);
        return false;
    }
    return true;
}

bool removeFile(const QString &path, QString *error)
{
    if (QFile::exists(path) && !QFile::remove(path)) {
        if (error)
            *error = QStringLiteral("cannot remove %1").arg(path);
        return false;
    }
    return true;
}
} // namespace

Paths standardPaths()
{
    return {QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
            QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)};
}

QString desktopFile(const Paths &paths)
{
    return paths.configHome + QStringLiteral("/autostart/io.github.diode701.NyaaPaper.desktop");
}

QString busServiceFile(const Paths &paths)
{
    return paths.dataHome + QStringLiteral("/dbus-1/services/org.nyaa.Paper.service");
}

bool apply(bool enable, const Paths &paths, const QString &guiPath, const QString &daemonPath, QString *error)
{
    if (!enable)
        return removeFile(desktopFile(paths), error) && removeFile(busServiceFile(paths), error);

    // Java's desktop entry, pointing at the GUI in tray mode
    const QByteArray desktop =
        "[Desktop Entry]\n"
        "Name=Nyaa Paper\n"
        "Comment=Wallpaper Engine frontend\n"
        "Exec=\"" + guiPath.toUtf8() + "\" --tray\n"
        "Icon=io.github.diode701.NyaaPaper\n"
        "Terminal=false\n"
        "Type=Application\n"
        "X-GNOME-Autostart-enabled=true\n";
    if (!writeFile(desktopFile(paths), desktop, error))
        return false;

    if (daemonPath.isEmpty())
        return true;
    const QByteArray service =
        "[D-BUS Service]\n"
        "Name=org.nyaa.Paper\n"
        "Exec=" + daemonPath.toUtf8() + "\n";
    return writeFile(busServiceFile(paths), service, error);
}

QString runningDaemonPath()
{
    QDBusConnectionInterface *bus = QDBusConnection::sessionBus().interface();
    if (!bus)
        return {};
    const auto pid = bus->servicePid(QStringLiteral("org.nyaa.Paper"));
    if (!pid.isValid() || pid.value() == 0)
        return {};
    return QFileInfo(QStringLiteral("/proc/%1/exe").arg(pid.value())).symLinkTarget();
}

} // namespace Autostart
