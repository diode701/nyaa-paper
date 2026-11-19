// ThumbnailCache: ids survive odd paths, decodes are centre-cropped to the
// requested size, and a second request is served from memory.
#include <cassert>
#include <cstdio>

#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

#include "ui/Thumbnails.hpp"

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QGuiApplication app(argc, argv);
    QTemporaryDir dir;

    // a wide picture: red left third, green middle, blue right third
    QImage wide(900, 300, QImage::Format_RGB32);
    {
        QPainter p(&wide);
        p.fillRect(0, 0, 300, 300, Qt::red);
        p.fillRect(300, 0, 300, 300, Qt::green);
        p.fillRect(600, 0, 300, 300, Qt::blue);
    }
    const QString path = dir.filePath("odd name #1 (preview).png");
    assert(wide.save(path));

    // id round trip, spaces, '#' and brackets included
    // previews arrive as QUrl::fromLocalFile(...).toString() (WallpaperModel)
    const QString id = ThumbnailCache::idFor(QUrl::fromLocalFile(path).toString());
    assert(id.startsWith("image://thumbs/"));
    assert(ThumbnailCache::pathOf(id) == path);
    assert(ThumbnailCache::pathOf(ThumbnailCache::idFor(path)) == path);   // a plain path too
    assert(ThumbnailCache::idFor("").isEmpty());

    ThumbnailCache cache;
    const QImage thumb = cache.get(path, QSize(180, 180));
    assert(thumb.size() == QSize(180, 180));
    // centre crop: what PreserveAspectCrop shows, the green middle
    const QColor centre = thumb.pixelColor(90, 90);
    assert(centre.green() > 200 && centre.red() < 50 && centre.blue() < 50);
    assert(cache.count() == 1);

    // second request: from memory, same pixels
    QElapsedTimer t;
    t.start();
    const QImage again = cache.get(path, QSize(180, 180));
    assert(again == thumb && t.elapsed() < 5);

    // prefetch fills the cache in the background
    QImage tall(200, 600, QImage::Format_RGB32);
    tall.fill(Qt::yellow);
    const QString other = dir.filePath("tall.png");
    assert(tall.save(other));
    cache.prefetch({ThumbnailCache::idFor(other)}, 180);
    for (int i = 0; i < 200 && cache.count() < 2; ++i)
        QThread::msleep(5);
    assert(cache.count() == 2);

    assert(cache.get(dir.filePath("missing.png"), QSize(180, 180)).isNull());
    std::puts("thumbnail tests passed");
    return 0;
}
