import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// VolumeOption → heading, Mute, slider 0–100 step 5, value (VolumeSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue

    Text { text: label; color: "#e0e0e0"; font.bold: true; font.pixelSize: 12 }
    RowLayout {
        spacing: 10
        CheckBox {
            text: "Mute"
            checked: boolValue === true
            onToggled: GlobalOptions.setValue(name, checked)
        }
        Slider {
            Layout.preferredWidth: 220
            from: 0; to: 100; stepSize: 5
            value: typeof settingValue === "number" ? settingValue : 70
            enabled: boolValue !== true
            onMoved: GlobalOptions.setValue(name, Math.round(value))
        }
        Text {
            text: String(settingValue)
            color: "#d3d3d3"
            font.pixelSize: 12
            opacity: boolValue === true ? 0.4 : 1
            Layout.preferredWidth: 34
        }
        Item { Layout.fillWidth: true }
    }
}
