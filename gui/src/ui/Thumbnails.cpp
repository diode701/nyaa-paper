#include "Thumbnails.hpp"

#include <QImageReader>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QUrl>

namespace {
constexpr int kDefaultEdge = 180;
// ~3 pages of a 12×6 grid at 2× density, or far more at 1×. In KiB.
constexpr int kCacheKiB = 192 * 1024;

class Response final : public QQuickImageResponse, public QRunnable
{
public:
    Response(std::shared_ptr<ThumbnailCache> cache, QString path, QSize size)
        : m_cache(std::move(cache)), m_path(std::move(path)), m_size(size)
    {
        setAutoDelete(false);
    }
    void run() override
    {
        m_image = m_cache->get(m_path, m_size);
        emit finished();
    }
    QQuickTextureFactory *textureFactory() const override
    {
        return QQuickTextureFactory::textureFactoryForImage(m_image);
    }

private:
    std::shared_ptr<ThumbnailCache> m_cache;
    QString m_path;
    QSize m_size;
    QImage m_image;
};
} // namespace

// ---- cache -----------------------------------------------------------------

ThumbnailCache::ThumbnailCache(QObject *parent)
    : QObject(parent)
{
    m_cache.setMaxCost(kCacheKiB);
    // background decoding leaves cores free for the UI and the visible page
    m_pool.setMaxThreadCount(qMax(1, QThread::idealThreadCount() / 2));
}

ThumbnailCache::~ThumbnailCache()
{
    m_pool.clear();
    m_pool.waitForDone();
}

QString ThumbnailCache::key(const QString &path, const QSize &size)
{
    return path + QLatin1Char('@') + QString::number(size.width()) + QLatin1Char('x')
           + QString::number(size.height());
}

QString ThumbnailCache::idFor(const QString &previewUrl)
{
    if (previewUrl.isEmpty())
        return {};
    const QUrl url(previewUrl);
    const QString path = url.isLocalFile() ? url.toLocalFile() : previewUrl;
    return QStringLiteral("image://thumbs/") + QString::fromLatin1(QUrl::toPercentEncoding(path));
}

QString ThumbnailCache::pathOf(const QString &id)
{
    QString encoded = id;
    if (encoded.startsWith(QLatin1String("image://thumbs/")))
        encoded = encoded.mid(15);
    return QUrl::fromPercentEncoding(encoded.toLatin1());
}

QImage ThumbnailCache::decode(const QString &path, const QSize &size)
{
    // decode straight at the size we need, then crop the centre square:
    // the same picture PreserveAspectCrop would show, at a fraction of the memory
    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize full = reader.size();
    if (full.isValid() && full.width() > 0 && full.height() > 0) {
        const double scale = qMax(double(size.width()) / full.width(), double(size.height()) / full.height());
        reader.setScaledSize(QSize(qMax(1, qRound(full.width() * scale)), qMax(1, qRound(full.height() * scale))));
    }
    QImage image = reader.read();
    if (image.isNull())
        return {};
    if (image.size() != size) {
        const QRect centre((image.width() - size.width()) / 2, (image.height() - size.height()) / 2,
                           size.width(), size.height());
        image = image.copy(centre.intersected(image.rect()));
    }
    return image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
}

QImage ThumbnailCache::get(const QString &path, const QSize &requested)
{
    const QSize size = requested.isValid() && requested.width() > 0
                           ? requested : QSize(kDefaultEdge, kDefaultEdge);
    const QString k = key(path, size);
    {
        QMutexLocker locker(&m_lock);
        if (const QImage *hit = m_cache.object(k))
            return *hit;
    }
    QImage image = decode(path, size);
    if (!image.isNull()) {
        QMutexLocker locker(&m_lock);
        m_cache.insert(k, new QImage(image), qMax<qsizetype>(1, image.sizeInBytes() / 1024));
    }
    return image;
}

void ThumbnailCache::prefetch(const QStringList &ids, int edge)
{
    const QSize size(edge, edge);
    for (const QString &id : ids) {
        const QString path = pathOf(id);
        if (path.isEmpty())
            continue;
        const QString k = key(path, size);
        {
            QMutexLocker locker(&m_lock);
            if (m_cache.contains(k) || m_pending.contains(k))
                continue;
            m_pending.insert(k);
        }
        m_pool.start([this, path, size, k] {
            get(path, size);
            QMutexLocker locker(&m_lock);
            m_pending.remove(k);
        });
    }
}

int ThumbnailCache::count() const
{
    QMutexLocker locker(&m_lock);
    return int(m_cache.count());
}

// ---- provider --------------------------------------------------------------

ThumbnailProvider::ThumbnailProvider(std::shared_ptr<ThumbnailCache> cache)
    : m_cache(std::move(cache))
{
    m_pool.setMaxThreadCount(qMax(2, QThread::idealThreadCount() / 2));
}

QQuickImageResponse *ThumbnailProvider::requestImageResponse(const QString &id, const QSize &requestedSize)
{
    auto *response = new Response(m_cache, ThumbnailCache::pathOf(id), requestedSize);
    m_pool.start(response);
    return response;
}
