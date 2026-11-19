#include "util/Gpu.h"

#include <QFileInfo>
#include <QProcess>

namespace nyaa::util {

bool detectNvidia()
{
    // The file checks are instant; nvidia-smi can take a moment.
    // /sys/module is hidden inside a Flatpak, /proc/driver/nvidia and the
    // device nodes (--device=dri) are not.
    for (const char *path : {"/sys/module/nvidia", "/proc/driver/nvidia/version", "/dev/nvidiactl"})
        if (QFileInfo::exists(QString::fromLatin1(path)))
            return true;
    QProcess smi;
    smi.start(QStringLiteral("nvidia-smi"), {});
    if (!smi.waitForStarted(500))
        return false;
    if (!smi.waitForFinished(3000)) {
        smi.kill();
        smi.waitForFinished(500);
        return false;
    }
    return smi.exitStatus() == QProcess::NormalExit && smi.exitCode() == 0;
}

} // namespace nyaa::util
