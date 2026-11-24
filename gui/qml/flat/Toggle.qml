import QtQuick
import NyaaPaper.Theme 1.0

/// On/off. Square, no glow: the knob moves, the track takes the accent.
Item {
    id: toggle
    property bool checked: false
    property string text: ""
    signal toggled(bool checked)
    implicitWidth: track.width + (text !== "" ? label.implicitWidth + 10 : 0)
    implicitHeight: 22
    activeFocusOnTab: true

    function flip() { toggled(!checked) }
    Keys.onSpacePressed: flip()

    Rectangle {
        id: track
        width: 34; height: 18
        anchors.verticalCenter: parent.verticalCenter
        radius: Theme.radius
        color: toggle.checked ? Theme.accent : Theme.line
        border.color: toggle.activeFocus ? Theme.accent2 : "transparent"
        Behavior on color { ColorAnimation { duration: 120 } }
        Rectangle {
            width: 12; height: 12; radius: 1
            y: 3
            x: toggle.checked ? parent.width - width - 3 : 3
            color: toggle.checked ? "#ffffff" : Theme.muted
            Behavior on x { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }
    }
    Text {
        id: label
        visible: toggle.text !== ""
        anchors.left: track.right
        anchors.leftMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: toggle.text
        color: Theme.muted
        font.family: Theme.font
        font.pixelSize: 12
    }
    MouseArea { anchors.fill: parent; onClicked: toggle.flip() }
}
