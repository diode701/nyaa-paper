import QtQuick
import QtQuick.Controls
import NyaaPaper.Backend 1.0

/// FlagOption → one checkbox with its label (BooleanFlagSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var boolValue

    CheckBox {
        text: label
        checked: boolValue === true
        font.pixelSize: 12
        onToggled: GlobalOptions.setValue(name, checked)
    }
}
