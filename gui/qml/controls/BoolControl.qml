import QtQuick
import NyaaPaper.Backend 1.0
import "../flat"

/// BooleanSetting (BooleanProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    caption: label
    Toggle { checked: settingValue === true; onToggled: c => Properties.setValue(name, c) }
}
