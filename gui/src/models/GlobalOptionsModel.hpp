#pragma once

#include <QAbstractListModel>
#include <QString>

#include "domain/options/GlobalOptions.h"

/// The Global Options tab. One row per domain GlobalOption, in
/// GlobalOptionsFactory order; the model owns the options it shows.
///
/// Java: WallpaperPropertiesView.populateGlobalSettings() built fresh options
/// for every wallpaper you opened, and their values travelled with that apply
/// (saved in the monitor's WallpaperConfig). Same here: reset() on open, the
/// controller captures and restores them.
class GlobalOptionsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount CONSTANT)

public:
    /// Which control the view draws: the Setting's own Kind.
    enum OptionKind {
        Volume = int(nyaa::domain::settings::Kind::Volume),
        Fps = int(nyaa::domain::settings::Kind::Fps),
        Scaling = int(nyaa::domain::settings::Kind::Scaling),
        Clamp = int(nyaa::domain::settings::Kind::Clamp),
        Flag = int(nyaa::domain::settings::Kind::Flag)
    };
    enum Role {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        KindRole,
        ValueRole,   // volume level, fps number, scaling name, clamp mode
        BoolRole,    // muted, fps limited, clamp enabled, flag on
        OptionsRole
    };

    explicit GlobalOptionsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// A bool toggles the row's checkbox; anything else sets its value.
    Q_INVOKABLE void setValue(const QString &name, const QVariant &value);

    const nyaa::domain::options::GlobalOptions &options() const { return m_options; }
    nyaa::domain::options::GlobalOptions &options() { return m_options; }
    /// Tell the view every row changed (after a restore through the list).
    void refresh() { refreshAll(); }

    /// Fresh defaults (a new wallpaper was opened).
    void reset();
    /// Apply saved name -> value pairs; unknown names are ignored.
    void restore(const std::map<std::string, std::string> &saved);

Q_SIGNALS:
    void changed();

private:
    const nyaa::domain::options::GlobalOption *optionAt(int row) const;
    void refreshAll();

    nyaa::domain::options::GlobalOptions m_options;
};
