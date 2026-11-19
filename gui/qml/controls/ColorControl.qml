import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

/// ColorSetting (ColorProperty.createControl): swatch and a colour picker
SettingCard {
    id: control
    required property string name
    required property string label
    required property var display      // "#RRGGBB"
    caption: label
    Rectangle {
        width: 24; height: 24; radius: Theme.radius
        border.color: Theme.line
        color: String(control.display).match(/^#[0-9a-fA-F]{6}$/) ? String(control.display) : "#000000"
        MouseArea { anchors.fill: parent; onClicked: picker.open() }
    }
    ActionButton { text: "Pick…"; onClicked: picker.open() }
    Text { text: String(control.display); color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
    Item { Layout.fillWidth: true }
    ColorDialog {
        id: picker
        title: control.label
        selectedColor: String(control.display)
        onAccepted: Properties.setValue(control.name, selectedColor.toString().substring(0, 7))
    }
}
