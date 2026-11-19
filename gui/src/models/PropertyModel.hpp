#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVariantList>

#include "domain/settings/SettingList.h"

/// The properties pane's data source: one row per Setting parsed from a
/// wallpaper's project.json. The view switches on `kind` to choose a delegate;
/// the model itself holds no UI logic.
class PropertyModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(QString wallpaper READ wallpaper NOTIFY countChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        KindRole,        // int, matches nyaa::domain::settings::Kind
        DisplayRole,
        ValueRole,       // bool / double / string, for bindings
        MinimumRole,
        MaximumRole,
        StepRole,
        OptionsRole,     // QStringList of labels, combo only
        IsGroupRole
    };

    explicit PropertyModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString wallpaper() const { return m_wallpaper; }

    Q_INVOKABLE void loadWallpaper(const QString &basePath);
    Q_INVOKABLE void setValue(const QString &name, const QVariant &value);

Q_SIGNALS:
    void countChanged();
    void changed();

private:
    nyaa::domain::settings::SettingList m_settings;
    QString m_wallpaper;
};
