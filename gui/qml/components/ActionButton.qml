import QtQuick
import QtQuick.Controls
import NyaaPaper.Theme 1.0

/// A flat button. `color` fills it (the accent for the main action);
/// `outline` draws it as a border in that colour instead.
Button {
    id: button

    property color color: Theme.raise
    property color textColor: outline ? color : (color === Theme.raise ? Theme.text : "#ffffff")
    property bool outline: false
    property bool bold: false
    property string keyHint: ""

    implicitHeight: 30
    implicitWidth: Math.max(64, row.implicitWidth + 24)
    font.family: Theme.font
    font.pixelSize: 12

    contentItem: Item {
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 8
            Rectangle {
                visible: button.keyHint !== ""
                anchors.verticalCenter: parent.verticalCenter
                width: hint.implicitWidth + 10; height: 18; radius: Theme.radius
                color: "#22000000"
                border.color: Qt.rgba(button.textColor.r, button.textColor.g, button.textColor.b, 0.3)
                Text { id: hint; anchors.centerIn: parent; text: button.keyHint; color: button.textColor; font.family: Theme.font; font.pixelSize: 10; font.weight: Font.DemiBold }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: button.text
                color: button.textColor
                font.family: Theme.font
                font.pixelSize: button.font.pixelSize
                font.weight: button.bold ? Font.Bold : Font.DemiBold
            }
        }
        opacity: button.enabled ? 1.0 : 0.4
    }

    background: Rectangle {
        radius: Theme.radius
        color: button.outline ? "transparent"
               : (button.down ? Qt.darker(button.color, 1.15) : (button.hovered ? Qt.lighter(button.color, 1.12) : button.color))
        border.width: 1
        border.color: button.outline ? button.color
                      : (button.hovered || button.activeFocus ? Qt.lighter(Theme.line, 1.6) : Theme.line)
    }
}
