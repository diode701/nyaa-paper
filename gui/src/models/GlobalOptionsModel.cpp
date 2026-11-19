#include "GlobalOptionsModel.hpp"

#include <QStringList>

using namespace nyaa::domain::options;

namespace {
QStringList toList(const std::vector<std::string> &values)
{
    QStringList out;
    for (const auto &value : values)
        out << QString::fromStdString(value);
    return out;
}
} // namespace

GlobalOptionsModel::GlobalOptionsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int GlobalOptionsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_options.all().size());
}

const GlobalOption *GlobalOptionsModel::optionAt(int row) const
{
    const auto all = m_options.all();
    return (row >= 0 && row < int(all.size())) ? all[size_t(row)] : nullptr;
}

QVariant GlobalOptionsModel::data(const QModelIndex &index, int role) const
{
    const GlobalOption *option = optionAt(index.row());
    if (!index.isValid() || !option)
        return {};

    const int kind = int(option->getKind());

    switch (role) {
    case NameRole:
        return QString::fromStdString(option->getName());
    case LabelRole:
        return QString::fromStdString(option->getDescription());
    case KindRole:
        return kind;
    case ValueRole:
        switch (kind) {
        case Volume: return m_options.volume().getVolume();
        case Fps: return m_options.fps().getFps();
        case Scaling: return QString::fromStdString(m_options.scaling().getScaling());
        case Clamp: return QString::fromStdString(m_options.clamp().getMode());
        default: return {};
        }
    case BoolRole:
        switch (kind) {
        case Volume: return m_options.volume().isMuted();
        case Fps: return m_options.fps().isLimited();
        case Clamp: return m_options.clamp().isEnabled();
        case Flag: return static_cast<const FlagOption *>(option)->isEnabled();
        default: return false;
        }
    case OptionsRole:
        // ScalingSetting.OPTIONS and ClampSetting.OPTIONS, in Java's order
        switch (kind) {
        case Scaling: return toList(ScalingOption::options());
        case Clamp: return toList(ClampOption::options());
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
    const bool isToggle = value.typeId() == QMetaType::Bool;
    const std::string key = name.toStdString();

    if (key == "volume") {
        if (isToggle) m_options.volume().setMuted(value.toBool());
        else m_options.volume().setVolume(value.toInt());
    } else if (key == "fps") {
        if (isToggle) m_options.fps().setLimited(value.toBool());
        else m_options.fps().setFps(value.toInt());
    } else if (key == "scaling") {
        m_options.scaling().setValueFromString(value.toString().toStdString());
    } else if (key == "clamp") {
        if (isToggle) m_options.clamp().setEnabled(value.toBool());
        else m_options.clamp().setMode(value.toString().toStdString());
    } else {
        m_options.setFlag(key, value.toBool());
    }
    refreshAll();
}

void GlobalOptionsModel::reset()
{
    m_options.reset();
    refreshAll();
}

void GlobalOptionsModel::restore(const std::map<std::string, std::string> &saved)
{
    m_options.restore(saved);
    refreshAll();
}

void GlobalOptionsModel::refreshAll()
{
    if (rowCount() > 0)
        emit dataChanged(index(0), index(rowCount() - 1));
    emit changed();
}
