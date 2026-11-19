import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../flat"

/// SliderSetting (SliderProperty.createControl): slider and two-decimal value
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var minimum
    required property var maximum
    required property var step
    caption: label
    FlatSlider {
        Layout.preferredWidth: 300
        from: minimum; to: maximum; stepSize: step
        value: typeof settingValue === "number" ? settingValue : 0
        onMoved: Properties.setValue(name, value)
    }
    Text { text: Number(settingValue).toFixed(2); color: Theme.text; font.family: Theme.font; font.pixelSize: 12; Layout.preferredWidth: 48 }
    Item { Layout.fillWidth: true }
}
