#include <cassert>
#include <cstdio>

#include "util/VideoDuration.h"

int main()
{
    // Missing file and non-video files must both come back as -1, never hang.
    assert(nyaa::util::videoDuration("/nonexistent/video.mp4") == -1.0);
    assert(nyaa::util::videoDuration("/etc/hostname") == -1.0);
    std::puts("video duration tests passed");
    return 0;
}
