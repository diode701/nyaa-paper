import QtQuick
import NyaaPaper.Theme 1.0

Rectangle {
    id: toast
    property string text: ""
    width: Math.min(420, message.implicitWidth + 40)
    height: message.implicitHeight + 28
    radius: Theme.radius
    color: Theme.panelAlt
    border.color: Theme.danger
    opacity: 0
    visible: opacity > 0

    function show(msg) {
        text = msg
        opacity = 1
        hideTimer.restart()
    }

    Behavior on opacity {
        NumberAnimation { duration: Theme.fast }
    }

    Timer {
        id: hideTimer
        interval: 4200
        onTriggered: toast.opacity = 0
    }

    Text {
        id: message
        anchors.fill: parent
        anchors.margins: 14
        text: toast.text
        color: Theme.text
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        verticalAlignment: Text.AlignVCenter
    }
}
