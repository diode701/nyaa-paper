import QtQuick
import NyaaPaper.Backend 1.0
import "../flat"

/// FlagOption (BooleanFlagSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var boolValue
    caption: label
    Toggle { checked: boolValue === true; onToggled: c => GlobalOptions.setValue(name, c) }
}
