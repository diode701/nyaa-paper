import QtQuick
import QtQuick.Controls
import NyaaPaper.Theme 1.0

/// A button that keeps its own chrome, independent of the control style.
Button {
    id: button

    property bool primary: false
    property color color: "#444444"
    property color textColor: "#ffffff"
    property bool bold: false

    implicitHeight: 30
    implicitWidth: Math.max(96, contentLabel.implicitWidth + 30)
    font.pixelSize: 12

    contentItem: Text {
        id: contentLabel
        text: button.text
        color: button.textColor
        font.pixelSize: button.font.pixelSize
        font.bold: button.bold
        font.family: button.font.family
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: button.enabled ? 1.0 : 0.4
    }

    background: Rectangle {
        color: button.down ? Qt.darker(button.color, 1.15)
                           : (button.hovered ? Qt.lighter(button.color, 1.2) : button.color)
        border.width: 1
        border.color: button.hovered ? Qt.lighter(button.color, 1.35) : Qt.darker(button.color, 1.1)
        radius: 3
        opacity: button.enabled ? 1.0 : 0.5
    }
}
