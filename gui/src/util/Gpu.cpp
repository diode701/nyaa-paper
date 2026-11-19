#include "util/Gpu.h"

#include <QFileInfo>
#include <QProcess>

namespace nyaa::util {

bool detectNvidia()
{
    // The module check is instant; nvidia-smi can take a moment.
    if (QFileInfo::exists(QStringLiteral("/sys/module/nvidia")))
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
