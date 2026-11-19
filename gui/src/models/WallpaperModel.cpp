#include "WallpaperModel.hpp"

#include "ui/Thumbnails.hpp"

namespace {
QString q(const std::string &value) { return QString::fromStdString(value); }
} // namespace

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

nyaa::domain::Wallpaper WallpaperModel::make(const QString &id, const QString &title, const QString &type,
                                             const QString &preview, const QString &basePath,
                                             const QString &file)
{
    nyaa::domain::Wallpaper w;
    w.id = id.toStdString();
    w.title = title.toStdString();
    w.type = type.toStdString();
    w.preview = preview.toStdString();
    w.basePath = basePath.toStdString();
    w.file = file.toStdString();
    return w;
}

const nyaa::domain::Wallpaper *WallpaperModel::find(const QString &id) const
{
    const auto it = m_byId.constFind(id);
    return it == m_byId.cend() ? nullptr : &m_all[*it];
}

void WallpaperModel::setAll(QVector<nyaa::domain::Wallpaper> items)
{
    beginResetModel();
    m_all = std::move(items);
    rebuild();
    endResetModel();
    emit countChanged();
}

void WallpaperModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    beginResetModel();
    m_filter = filter;
    rebuild();
    endResetModel();
    emit filterChanged();
    emit countChanged();
}

void WallpaperModel::rebuild()
{
    m_byId.clear();
    m_visible.clear();
    m_visibleRow.clear();
    m_visible.reserve(m_all.size());
    for (int i = 0; i < m_all.size(); ++i) {
        m_byId.insert(q(m_all[i].id), i);
        if (m_filter.isEmpty() || q(m_all[i].title).contains(m_filter, Qt::CaseInsensitive)) {
            m_visibleRow.insert(q(m_all[i].id), int(m_visible.size()));
            m_visible.push_back(i);
        }
    }
}

int WallpaperModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_visible.size());
}

QVariant WallpaperModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_visible.size())
        return {};
    const nyaa::domain::Wallpaper &item = m_all[m_visible[index.row()]];
    switch (role) {
    case IdRole:      return q(item.id);
    case TitleRole:   return q(item.title);
    case TypeRole:    return q(item.type);
    case PreviewRole: return q(item.preview);
    case BasePathRole: return q(item.basePath);
    default:          return {};
    }
}

QHash<int, QByteArray> WallpaperModel::roleNames() const
{
    return {{IdRole, "wallpaperId"}, {TitleRole, "title"}, {TypeRole, "kind"}, {PreviewRole, "preview"}, {BasePathRole, "basePath"}};
}

QString WallpaperModel::idAt(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    return q(m_all[m_visible[row]].id);
}

QString WallpaperModel::titleAt(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    return q(m_all[m_visible[row]].title);
}

QVariantMap WallpaperModel::at(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= m_visible.size())
        return out;
    const nyaa::domain::Wallpaper &item = m_all[m_visible[row]];
    out.insert(QStringLiteral("id"), q(item.id));
    out.insert(QStringLiteral("title"), q(item.title));
    out.insert(QStringLiteral("kind"), q(item.type));
    out.insert(QStringLiteral("preview"), q(item.preview));
    out.insert(QStringLiteral("thumb"), ThumbnailCache::idFor(q(item.preview)));
    out.insert(QStringLiteral("basePath"), q(item.basePath));
    return out;
}

QString WallpaperModel::previewFor(const QString &id) const
{
    const auto it = m_byId.constFind(id);
    return it == m_byId.cend() ? QString() : q(m_all[*it].preview);
}

QString WallpaperModel::basePathFor(const QString &id) const
{
    const auto it = m_byId.constFind(id);
    return it == m_byId.cend() ? QString() : q(m_all[*it].basePath);
}

int WallpaperModel::indexOf(const QString &id) const
{
    return m_visibleRow.value(id, -1);
}

QString WallpaperModel::titleFor(const QString &id) const
{
    const auto it = m_byId.constFind(id);
    return it == m_byId.cend() ? id : q(m_all[*it].title);
}
