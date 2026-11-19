import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// SliderSetting → label, slider, value with two decimals (SliderProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var minimum
    required property var maximum
    required property var step

    RowLayout {
        spacing: 10
        Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
        Slider {
            Layout.preferredWidth: 220
            from: minimum
            to: maximum
            stepSize: step
            value: typeof settingValue === "number" ? settingValue : 0
            onMoved: Properties.setValue(name, value)
        }
        Text {
            text: Number(settingValue).toFixed(2)
            color: "#d3d3d3"
            font.pixelSize: 12
            Layout.preferredWidth: 44
        }
        Item { Layout.fillWidth: true }
    }
}
