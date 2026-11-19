import QtQuick
import QtQuick.Layouts
import NyaaPaper.Theme 1.0

/// GroupSetting (GroupProperty.createControl): a section header
SettingCard {
    required property string label
    group: true
    Text {
        text: label.toUpperCase()
        color: Theme.accent
        font.family: Theme.font
        font.pixelSize: 11
        font.weight: Font.Bold
        font.letterSpacing: 1.4
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        Layout.topMargin: 8
    }
}
