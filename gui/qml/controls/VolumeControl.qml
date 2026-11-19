import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../flat"

/// VolumeOption (VolumeSetting.createControl): mute, level 0–100 in steps of 5
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue
    caption: label
    Toggle { text: "Mute"; checked: boolValue === true; onToggled: c => GlobalOptions.setValue(name, c) }
    FlatSlider {
        Layout.preferredWidth: 240
        from: 0; to: 100; stepSize: 5
        value: typeof settingValue === "number" ? settingValue : 70
        enabled: boolValue !== true
        onMoved: GlobalOptions.setValue(name, Math.round(value))
    }
    Text { text: String(settingValue); color: boolValue === true ? Theme.dim : Theme.text; font.family: Theme.font; font.pixelSize: 12 }
    Item { Layout.fillWidth: true }
}
