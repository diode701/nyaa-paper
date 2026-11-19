#pragma once

class QApplication;
class QQmlApplicationEngine;

/// Environment switches for screenshots and smoke tests; no effect unless set.
///   NYAA_WINDOW=WxH      window size        NYAA_VIEW=1|2|3  open a view
///   NYAA_GLOBAL / NYAA_MINI  open the Global tab / mini panel
///   NYAA_KEYS=a,b,c      inject keys, print focus/cursor/page, quit
///   NYAA_SHOT=file.png   grab one frame and quit (NYAA_SHOT_DELAY ms)
void installDevHooks(QApplication &app, QQmlApplicationEngine &qml);
