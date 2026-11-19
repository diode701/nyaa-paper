#include "PropertyModel.hpp"

#include "domain/settings/SettingFactory.h"
#include "domain/settings/Settings.h"
#include "io/ProjectJson.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace {
using nyaa::domain::settings::ComboSetting;
using nyaa::domain::settings::SliderSetting;
} // namespace

PropertyModel::PropertyModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PropertyModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_settings.all().size());
}

QVariant PropertyModel::data(const QModelIndex &index, int role) const
{
    const auto &all = m_settings.all();
    if (!index.isValid() || index.row() < 0 || index.row() >= int(all.size()))
        return {};

    const auto *setting = all[size_t(index.row())].get();
    switch (role) {
    case NameRole:
        return QString::fromStdString(setting->getName());
    case LabelRole:
        return QString::fromStdString(setting->getDescription());
    case KindRole:
        return int(setting->getKind());
    case DisplayRole:
        if (const auto *color = dynamic_cast<const nyaa::domain::settings::ColorSetting *>(setting))
            return QString::fromStdString(color->getHex());
        return QString::fromStdString(setting->getValueString());
    case ValueRole: {
        if (const auto *slider = dynamic_cast<const SliderSetting *>(setting))
            return slider->value();
        if (const auto *combo = dynamic_cast<const ComboSetting *>(setting))
            return combo->value();
        return QString::fromStdString(setting->getValueString());
    }
    case MinimumRole: {
        if (const auto *slider = dynamic_cast<const SliderSetting *>(setting))
            return slider->getRange().min;
        return 0.0;
    }
    case MaximumRole: {
        if (const auto *slider = dynamic_cast<const SliderSetting *>(setting))
            return slider->getRange().max;
        return 1.0;
    }
    case StepRole: {
        if (const auto *slider = dynamic_cast<const SliderSetting *>(setting))
            return slider->getRange().step;
        return 0.01;
    }
    case OptionsRole: {
        QStringList labels;
        if (const auto *combo = dynamic_cast<const ComboSetting *>(setting)) {
            for (const auto &option : combo->getOptions())
                labels << QString::fromStdString(option.second);
        }
        return labels;
    }
    case IsGroupRole:
        return setting->getKind() == nyaa::domain::settings::Kind::Group;
    default:
        return {};
    }
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
    return {{NameRole, "name"},      {LabelRole, "label"},     {KindRole, "kind"},
            {DisplayRole, "display"}, {ValueRole, "settingValue"},     {MinimumRole, "minimum"},
            {MaximumRole, "maximum"}, {StepRole, "step"},       {OptionsRole, "options"},
            {IsGroupRole, "isGroup"}};
}

void PropertyModel::loadWallpaper(const QString &basePath)
{
    beginResetModel();
    m_settings = nyaa::domain::settings::SettingList{};
    m_wallpaper = basePath;
    const std::string project = basePath.toStdString() + "/project.json";
    for (const auto &desc : nyaa::io::parseProjectFile(project))
        m_settings.add(nyaa::domain::settings::makeSetting(desc));
    endResetModel();
    emit countChanged();
}

void PropertyModel::setValue(const QString &name, const QVariant &value)
{
    auto *setting = m_settings.find(name.toStdString());
    if (!setting)
        return;

    const auto kind = setting->getKind();
    if (kind == nyaa::domain::settings::Kind::Bool) {
        setting->setValueFromString(value.toBool() ? "1" : "0");
    } else if (kind == nyaa::domain::settings::Kind::Slider) {
        setting->setValueFromString(std::to_string(value.toDouble()));
    } else if (kind == nyaa::domain::settings::Kind::Combo) {
        // The combo delegate hands us the label; translate it to its value.
        if (auto *combo = dynamic_cast<ComboSetting *>(setting)) {
            const QString label = value.toString();
            for (const auto &option : combo->getOptions()) {
                if (QString::fromStdString(option.second) == label) {
                    setting->setValueFromString(std::to_string(option.first));
                    break;
                }
            }
        }
    } else {
        setting->setValueFromString(value.toString().toStdString());
    }

    const auto &all = m_settings.all();
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].get() == setting) {
            const QModelIndex idx = index(int(i));
            emit dataChanged(idx, idx);
            break;
        }
    }
    emit changed();
}

QVariantMap PropertyModel::values() const
{
    QVariantMap out;
    for (const auto &setting : m_settings.all())
        out.insert(QString::fromStdString(setting->getName()),
                   QString::fromStdString(setting->getValueString()));
    return out;
}

void PropertyModel::applyValues(const QVariantMap &values)
{
    for (auto it = values.begin(); it != values.end(); ++it) {
        auto *setting = m_settings.find(it.key().toStdString());
        if (setting)
            setting->setValueFromString(it.value().toString().toStdString());
    }
    if (!m_settings.all().empty()) {
        emit dataChanged(index(0), index(int(m_settings.all().size()) - 1));
    }
    emit changed();
}
