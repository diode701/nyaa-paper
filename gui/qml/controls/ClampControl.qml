import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// ClampOption → heading, "Enable clamping", mode combo (ClampSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue
    required property var options

    Text { text: label; color: "#e0e0e0"; font.bold: true; font.pixelSize: 12 }
    RowLayout {
        spacing: 10
        CheckBox {
            text: "Enable clamping"
            checked: boolValue === true
            onToggled: GlobalOptions.setValue(name, checked)
        }
        ComboBox {
            Layout.preferredWidth: 160
            implicitHeight: 30
            font.pixelSize: 12
            model: options
            currentIndex: Math.max(0, options.indexOf(String(settingValue)))
            enabled: boolValue === true
            onActivated: GlobalOptions.setValue(name, currentText)
        }
        Item { Layout.fillWidth: true }
    }
}
