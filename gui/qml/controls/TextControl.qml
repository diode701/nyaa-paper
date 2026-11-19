import QtQuick
import QtQuick.Layouts
import NyaaPaper.Theme 1.0

/// TextSetting (TextProperty.createControl): a read-only note across the row
SettingCard {
    required property string label
    color: "transparent"
    Text {
        text: label
        color: Theme.dim
        font.family: Theme.font
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
}
