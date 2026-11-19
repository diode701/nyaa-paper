import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../flat"

/// ClampOption (ClampSetting.createControl): on/off and the mode
SettingCard {
    id: control
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue
    required property var options
    caption: label
    Toggle { checked: control.boolValue === true; onToggled: c => GlobalOptions.setValue(control.name, c) }
    Segmented {
        options: control.options
        value: String(control.settingValue)
        opacity: control.boolValue === true ? 1 : 0.4
        enabled: control.boolValue === true
        onPicked: v => GlobalOptions.setValue(control.name, v)
    }
    Item { Layout.fillWidth: true }
}
