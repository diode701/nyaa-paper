#pragma once

#include <memory>

#include <QCache>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QQuickAsyncImageProvider>
#include <QSet>
#include <QStringList>
#include <QThreadPool>

/// Decoded grid thumbnails, owned by the app instead of Qt's pixmap cache.
///
/// Java: WallpaperBrowser.imageCache + preloadAdjacentPages + trimCache. Qt's
/// own cache keeps only ~2 MB of images nobody shows, so the page you just
/// left was thrown away and decoded again on the way back. Here every preview
/// is decoded once, centre-cropped to the card size, on worker threads, and
/// kept (least recently used first out) while it is near the current page.
///
/// QML: Image { source: "image://thumbs/<encoded path>" }  and
///      Thumbs.prefetch([...]) for the neighbouring pages.
class ThumbnailCache : public QObject
{
    Q_OBJECT

public:
    explicit ThumbnailCache(QObject *parent = nullptr);
    ~ThumbnailCache() override;

    /// The image for a path at a size, decoding it if needed. Thread safe.
    QImage get(const QString &path, const QSize &size);
    /// Decode these in the background so a later get() is instant.
    /// Entries are "image://thumbs/..." ids or plain paths.
    Q_INVOKABLE void prefetch(const QStringList &ids, int edge);
    /// "image://thumbs/<path>" for a preview URL or path ("" stays "").
    Q_INVOKABLE static QString idFor(const QString &previewUrl);
    static QString pathOf(const QString &id);
    int count() const;

private:
    static QImage decode(const QString &path, const QSize &size);
    static QString key(const QString &path, const QSize &size);

    mutable QMutex m_lock;
    QCache<QString, QImage> m_cache;   // cost in KiB
    QSet<QString> m_pending;
    QThreadPool m_pool;
};

/// The image provider QML talks to; answers from the cache on a worker thread.
class ThumbnailProvider : public QQuickAsyncImageProvider
{
public:
    explicit ThumbnailProvider(std::shared_ptr<ThumbnailCache> cache);
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    std::shared_ptr<ThumbnailCache> m_cache;
    QThreadPool m_pool;
};
