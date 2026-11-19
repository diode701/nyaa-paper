#pragma once

#include <string>

namespace nyaa::util {

/// Duration of a video file in seconds via ffprobe, or -1 when the file is
/// missing, ffprobe is absent, or the probe fails. Never throws, never blocks
/// longer than timeoutMs.
double videoDuration(const std::string &path, int timeoutMs = 5000);

} // namespace nyaa::util
