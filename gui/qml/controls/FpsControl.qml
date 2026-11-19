import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../flat"

/// FpsOption (FpsSetting.createControl): limit on/off and the number
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var boolValue
    caption: label
    Toggle { text: "Limit"; checked: boolValue === true; onToggled: c => GlobalOptions.setValue(name, c) }
    Stepper {
        from: 0; to: 240; step: 5; suffix: " fps"
        value: typeof settingValue === "number" ? settingValue : 30
        enabled: boolValue === true
        onModified: v => GlobalOptions.setValue(name, v)
    }
    Item { Layout.fillWidth: true }
}
