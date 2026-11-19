#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class ConfigBridge;

/// One row per engine-wide option. Values live in the shared config; the model
/// is a view over ConfigBridge so both stay in step.
class GlobalOptionsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    /// Mirrors the delegate choice in QML.
    enum OptionKind { Volume = 0, Fps = 1, Scaling = 2, Clamp = 3, Layer = 4, Flag = 5 };
    enum Role {
        NameRole = Qt::UserRole + 1,
        LabelRole,
        KindRole,
        ValueRole,
        BoolRole,
        OptionsRole
    };

    explicit GlobalOptionsModel(ConfigBridge *config, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void setValue(const QString &name, const QVariant &value);
    Q_INVOKABLE void reload();

Q_SIGNALS:
    void countChanged();

private:
    struct Row {
        QString name;
        QString label;
        int kind;
    };

    ConfigBridge *m_config = nullptr;
    QVector<Row> m_rows;
};
