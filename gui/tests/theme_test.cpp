// Theme colour sources: built-in palettes, from an image, from pywal.
#include <cassert>
#include <cmath>
#include <cstdio>

#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>

#include "ui/Theme.hpp"

namespace {
double hueDistance(double a, double b)
{
    const double d = std::fabs(a - b);
    return std::min(d, 360.0 - d);
}
} // namespace

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    QTemporaryDir dir;

    // nine palettes, Nyaa first, every one dark with readable text
    const auto &all = Theme::builtIn();
    assert(all.size() == 9 && all.first().key == "nyaa");
    for (const auto &p : all) {
        assert(p.bg.lightnessF() < 0.12);
        assert(p.text.lightnessF() > 0.7);
        assert(p.accent.isValid() && p.accent2.isValid());
    }

    // a mostly dark-teal picture with a small vivid orange patch:
    // the ground follows the teal, the accent picks the orange
    QImage picture(200, 200, QImage::Format_RGB32);
    picture.fill(QColor::fromHsv(185, 140, 90));
    {
        QPainter painter(&picture);
        painter.fillRect(140, 140, 50, 50, QColor::fromHsv(25, 255, 255));
    }
    const QString path = dir.filePath("wall.png");
    assert(picture.save(path));
    const Theme::Palette fromWall = Theme::fromImage(path);
    assert(fromWall.key == "wallpaper");
    assert(hueDistance(fromWall.bg.hslHue(), 185) < 15);
    assert(hueDistance(fromWall.accent.hslHue(), 25) < 15);
    assert(fromWall.bg.lightnessF() < 0.1);

    // a grey picture still gives a usable, quiet palette
    QImage grey(64, 64, QImage::Format_RGB32);
    grey.fill(QColor(120, 120, 120));
    assert(grey.save(dir.filePath("grey.png")));
    const Theme::Palette quiet = Theme::fromImage(dir.filePath("grey.png"));
    assert(quiet.key == "wallpaper" && quiet.accent.hslSaturationF() < 0.4);

    // pywal: background kept dark, the most saturated colour is the accent
    {
        QFile wal(dir.filePath("colors.json"));
        assert(wal.open(QIODevice::WriteOnly));
        wal.write(R"({"special": {"background": "#e8e0d0", "foreground": "#202020"},
                      "colors": {"color1": "#a05050", "color2": "#50a050", "color3": "#a0a050",
                                 "color4": "#2060ff", "color5": "#806080", "color6": "#608080"}})");
    }
    const Theme::Palette wal = Theme::fromPywal(dir.path());
    assert(wal.key == "pywal");
    assert(wal.bg.lightnessF() < 0.12);           // a light wal background is darkened
    assert(wal.accent == QColor("#2060ff"));
    assert(Theme::fromPywal(dir.filePath("missing")).key.isEmpty());

    assert(!Theme::fromImage(dir.filePath("nope.png")).key.size());
    std::puts("theme tests passed");
    return 0;
}
