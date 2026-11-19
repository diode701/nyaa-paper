import QtQuick
import QtQuick.Layouts

/// GroupSetting → a bold section header on #333 (GroupProperty.createControl)
SettingCard {
    required property string label
    group: true

    Text {
        text: label
        color: "#e0e0e0"
        font.pixelSize: 14
        font.bold: true
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
