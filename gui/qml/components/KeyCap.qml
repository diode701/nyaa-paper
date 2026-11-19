import QtQuick
import NyaaPaper.Theme 1.0

Rectangle {
    id: cap
    property string text: ""
    implicitWidth: label.implicitWidth + 12
    implicitHeight: 18
    radius: 4
    color: "transparent"
    border.color: Theme.line

    Text {
        id: label
        anchors.centerIn: parent
        text: cap.text
        color: Theme.muted
        font.pixelSize: 9
        font.family: Theme.mono
        font.letterSpacing: 1
    }
}
