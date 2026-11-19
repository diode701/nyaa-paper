#include "WallpaperModel.hpp"

WallpaperModel::WallpaperModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void WallpaperModel::setAll(QVector<WallpaperItem> items)
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
    m_visible.clear();
    m_visible.reserve(m_all.size());
    for (int i = 0; i < m_all.size(); ++i) {
        if (m_filter.isEmpty() || m_all[i].title.contains(m_filter, Qt::CaseInsensitive))
            m_visible.push_back(i);
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
    const WallpaperItem &item = m_all[m_visible[index.row()]];
    switch (role) {
    case IdRole:      return item.id;
    case TitleRole:   return item.title;
    case TypeRole:    return item.type;
    case PreviewRole: return item.preview;
    case BasePathRole: return item.basePath;
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
    return m_all[m_visible[row]].id;
}

QString WallpaperModel::titleAt(int row) const
{
    if (row < 0 || row >= m_visible.size())
        return {};
    return m_all[m_visible[row]].title;
}

QVariantMap WallpaperModel::at(int row) const
{
    QVariantMap out;
    if (row < 0 || row >= m_visible.size())
        return out;
    const WallpaperItem &item = m_all[m_visible[row]];
    out.insert(QStringLiteral("id"), item.id);
    out.insert(QStringLiteral("title"), item.title);
    out.insert(QStringLiteral("kind"), item.type);
    out.insert(QStringLiteral("preview"), item.preview);
    out.insert(QStringLiteral("basePath"), item.basePath);
    return out;
}
