import QtQuick
import NyaaPaper.Theme 1.0

/// A key, drawn like one: flat, square, monospace.
Rectangle {
    id: cap
    property string text: ""
    implicitWidth: Math.max(20, label.implicitWidth + 12)
    implicitHeight: 18
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line

    Text {
        id: label
        anchors.centerIn: parent
        text: cap.text
        color: Theme.muted
        font.family: Theme.font
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 0.5
    }
}
