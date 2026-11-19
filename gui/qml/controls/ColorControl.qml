import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../components"

/// ColorSetting → label and a colour picker (ColorProperty.createControl)
SettingCard {
    id: control
    required property string name
    required property string label
    required property var display      // "#RRGGBB"

    Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
    RowLayout {
        spacing: 10
        Rectangle {
            width: 28; height: 28; radius: 3
            border.color: "#555555"
            color: String(display).match(/^#[0-9a-fA-F]{6}$/) ? String(display) : "#000000"
            MouseArea { anchors.fill: parent; onClicked: picker.open() }
        }
        ActionButton { text: "Pick…"; onClicked: picker.open() }
        Text { text: String(display); color: "#aaaaaa"; font.pixelSize: 11 }
        Item { Layout.fillWidth: true }
    }
    ColorDialog {
        id: picker
        title: control.label
        selectedColor: String(control.display)
        onAccepted: Properties.setValue(control.name, selectedColor.toString().substring(0, 7))
    }
}
