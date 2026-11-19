#include "util/VideoDuration.h"

#include <QProcess>
#include <QStringList>

namespace nyaa::util {

double videoDuration(const std::string &path, int timeoutMs)
{
    QProcess probe;
    probe.start(QStringLiteral("ffprobe"),
                {QStringLiteral("-v"), QStringLiteral("error"),
                 QStringLiteral("-show_entries"), QStringLiteral("format=duration"),
                 QStringLiteral("-of"), QStringLiteral("default=noprint_wrappers=1:nokey=1"),
                 QString::fromStdString(path)});

    if (!probe.waitForStarted(500))
        return -1.0;
    if (!probe.waitForFinished(timeoutMs)) {
        probe.kill();
        probe.waitForFinished(500);
        return -1.0;
    }
    if (probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0)
        return -1.0;

    bool ok = false;
    const double seconds = QString::fromUtf8(probe.readAllStandardOutput()).trimmed().toDouble(&ok);
    return ok && seconds > 0.0 ? seconds : -1.0;
}

} // namespace nyaa::util
