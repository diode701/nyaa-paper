#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantMap>
#include <QVector>

/// One row of the library, as the daemon reports it.
struct WallpaperItem
{
    QString id;
    QString title;
    QString type;
    QString preview;
};

/// The grid's data source. Filtering lives here so the view stays dumb.
class WallpaperModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, TypeRole, PreviewRole };

    explicit WallpaperModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setAll(QVector<WallpaperItem> items);

    QString filter() const { return m_filter; }
    void setFilter(const QString &filter);
    int count() const { return int(m_visible.size()); }

    Q_INVOKABLE QString idAt(int row) const;
    Q_INVOKABLE QString titleAt(int row) const;
    /// One row as {id,title,kind,preview} for the QML page deck.
    Q_INVOKABLE QVariantMap at(int row) const;

Q_SIGNALS:
    void filterChanged();
    void countChanged();

private:
    void rebuild();

    QVector<WallpaperItem> m_all;
    QVector<int> m_visible;
    QString m_filter;
};
