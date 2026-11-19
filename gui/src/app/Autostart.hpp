#pragma once

#include <QString>

/// NyaaLauncher.applyAutoStart, for two processes.
///
/// Enabled: a login entry that starts the GUI hidden in the tray (Java ran
/// the launcher with --tray), and a D-Bus service file so the session bus
/// starts nyaa-paperd the first time the GUI calls it. Disabled: both removed.
namespace Autostart {

struct Paths {
    QString configHome;   // $XDG_CONFIG_HOME, for autostart/
    QString dataHome;     // $XDG_DATA_HOME, for dbus-1/services/
};

/// The user's standard locations.
Paths standardPaths();

QString desktopFile(const Paths &paths);
QString busServiceFile(const Paths &paths);

/// Write or remove both files. guiPath / daemonPath are executables; an empty
/// daemonPath skips the service file (the daemon is then started some other
/// way). Returns false with a readable reason on failure.
bool apply(bool enable, const Paths &paths, const QString &guiPath, const QString &daemonPath,
           QString *error = nullptr);

/// The executable that currently owns org.nyaa.Paper on the session bus,
/// read from /proc. Empty when no daemon runs.
QString runningDaemonPath();

} // namespace Autostart
