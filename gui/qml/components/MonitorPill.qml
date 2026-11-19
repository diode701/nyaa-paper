import QtQuick
import NyaaPaper.Theme 1.0

Rectangle {
    id: pill
    property string monitor: ""
    property bool running: false
    implicitWidth: label.implicitWidth + 34
    implicitHeight: 26
    radius: 13
    color: pill.running ? Theme.accentDim : "transparent"
    border.color: pill.running ? Theme.accent : Theme.line

    Rectangle {
        width: 6
        height: 6
        radius: 3
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 12
        color: pill.running ? Theme.accent : Theme.dim
    }

    Text {
        id: label
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 24
        text: pill.monitor
        color: pill.running ? Theme.accent : Theme.muted
        font.pixelSize: 10
        font.family: Theme.mono
        font.letterSpacing: 1
    }
}
