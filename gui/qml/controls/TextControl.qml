import QtQuick
import QtQuick.Layouts

/// TextSetting → a wrapped grey label, no value (TextProperty.createControl)
SettingCard {
    required property string label

    Text {
        text: label
        color: "#aaaaaa"
        font.pixelSize: 13
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
