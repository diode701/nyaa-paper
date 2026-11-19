#include "GlobalOptionsModel.hpp"

#include "bridge/ConfigBridge.hpp"

GlobalOptionsModel::GlobalOptionsModel(ConfigBridge *config, QObject *parent)
    : QAbstractListModel(parent)
    , m_config(config)
{
    m_rows = {
        {QStringLiteral("volume"), QStringLiteral("Volume"), Volume},
        {QStringLiteral("fps"), QStringLiteral("FPS limit"), Fps},
        {QStringLiteral("scaling"), QStringLiteral("Scaling"), Scaling},
        {QStringLiteral("clamp"), QStringLiteral("Texture clamp"), Clamp},
        {QStringLiteral("layer"), QStringLiteral("Wayland layer"), Layer},
        {QStringLiteral("disable-mouse"), QStringLiteral("Disable mouse interaction"), Flag},
        {QStringLiteral("disable-parallax"), QStringLiteral("Disable parallax"), Flag},
        {QStringLiteral("disable-particles"), QStringLiteral("Disable particles"), Flag},
        {QStringLiteral("noautomute"), QStringLiteral("Don't auto-mute"), Flag},
        {QStringLiteral("no-audio-processing"), QStringLiteral("Disable audio processing"), Flag},
        {QStringLiteral("no-fullscreen-pause"), QStringLiteral("Don't pause on fullscreen"), Flag},
        {QStringLiteral("fullscreen-pause-only-active"), QStringLiteral("Pause only when active fullscreen (Wayland)"), Flag},
    };
}

int GlobalOptionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QVariant GlobalOptionsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};

    const Row &row = m_rows[index.row()];
    switch (role) {
    case NameRole:
        return row.name;
    case LabelRole:
        return row.label;
    case KindRole:
        return row.kind;
    case ValueRole:
        switch (row.kind) {
        case Volume: return m_config->volume();
        case Fps: return m_config->fps();
        case Scaling: return m_config->scaling();
        case Clamp: return m_config->clampMode();
        case Layer: return m_config->layer();
        default: return {};
        }
    case BoolRole:
        switch (row.kind) {
        case Volume: return m_config->muted();
        case Fps: return m_config->fpsLimited();
        case Flag: return m_config->flagEnabled(row.name);
        default: return false;
        }
    case OptionsRole:
        switch (row.kind) {
        case Scaling: return QStringList{QStringLiteral("default"), QStringLiteral("stretch"),
                                         QStringLiteral("fit"), QStringLiteral("fill")};
        case Clamp: return QStringList{QString(), QStringLiteral("clamp"),
                                       QStringLiteral("border"), QStringLiteral("repeat")};
        case Layer: return QStringList{QString(), QStringLiteral("background"),
                                       QStringLiteral("bottom"), QStringLiteral("top"),
                                       QStringLiteral("overlay")};
        default: return QStringList{};
        }
    default:
        return {};
    }
}

QHash<int, QByteArray> GlobalOptionsModel::roleNames() const
{
    return {{NameRole, "name"}, {LabelRole, "label"}, {KindRole, "kind"},
            {ValueRole, "settingValue"}, {BoolRole, "boolValue"}, {OptionsRole, "options"}};
}

void GlobalOptionsModel::setValue(const QString &name, const QVariant &value)
{
    if (name == QStringLiteral("volume")) {
        if (value.typeId() == QMetaType::Bool)
            m_config->setMuted(value.toBool());
        else
            m_config->setVolume(value.toInt());
    } else if (name == QStringLiteral("fps")) {
        if (value.typeId() == QMetaType::Bool)
            m_config->setFpsLimited(value.toBool());
        else
            m_config->setFps(value.toInt());
    } else if (name == QStringLiteral("scaling")) {
        m_config->setScaling(value.toString());
    } else if (name == QStringLiteral("clamp")) {
        m_config->setClampMode(value.toString());
    } else if (name == QStringLiteral("layer")) {
        m_config->setLayer(value.toString());
    } else {
        m_config->setFlag(name, value.toBool());
    }

    m_config->save();
    emit dataChanged(index(0), index(int(m_rows.size()) - 1));
}

void GlobalOptionsModel::reload()
{
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(int(m_rows.size()) - 1));
}
