import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../flat"

/// ScalingOption (ScalingSetting.createControl)
SettingCard {
    id: control
    required property string name
    required property string label
    required property var settingValue
    required property var options
    caption: label
    Segmented { options: control.options; value: String(control.settingValue); onPicked: v => GlobalOptions.setValue(control.name, v) }
    Item { Layout.fillWidth: true }
}
