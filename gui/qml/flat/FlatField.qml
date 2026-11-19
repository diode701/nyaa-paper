import QtQuick
import NyaaPaper.Theme 1.0

/// A one-line text field: dark well, monospace, accent border when focused.
Rectangle {
    id: field
    property alias text: input.text
    property string placeholder: ""
    signal edited(string text)
    implicitWidth: 360
    implicitHeight: 28
    radius: Theme.radius
    color: Theme.bg
    border.color: input.activeFocus ? Theme.accent : Theme.line

    TextInput {
        id: input
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        verticalAlignment: TextInput.AlignVCenter
        color: Theme.text
        selectionColor: Theme.accent
        font.family: Theme.font
        font.pixelSize: 12
        clip: true
        selectByMouse: true
        onEditingFinished: field.edited(text)
        Text {
            anchors.verticalCenter: parent.verticalCenter
            visible: input.text === "" && !input.activeFocus
            text: field.placeholder
            color: Theme.dim
            font: input.font
        }
    }
}
