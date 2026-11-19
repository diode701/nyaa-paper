import QtQuick
import QtQuick.Controls
import NyaaPaper.Backend 1.0

/// BooleanSetting → a checkbox carrying its own label (BooleanProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue

    CheckBox {
        text: label
        checked: settingValue === true
        font.pixelSize: 12
        onToggled: Properties.setValue(name, checked)
    }
}
