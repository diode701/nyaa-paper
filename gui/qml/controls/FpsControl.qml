import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// FpsOption → heading, "Limit FPS", spinner 0–240 (FpsSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue

    Text { text: label; color: "#e0e0e0"; font.bold: true; font.pixelSize: 12 }
    RowLayout {
        spacing: 10
        CheckBox {
            text: "Limit FPS"
            checked: boolValue === true
            onToggled: GlobalOptions.setValue(name, checked)
        }
        SpinBox {
            from: 0; to: 240
            editable: true
            value: typeof settingValue === "number" ? settingValue : 30
            enabled: boolValue === true
            onValueModified: GlobalOptions.setValue(name, value)
        }
        Item { Layout.fillWidth: true }
    }
}
