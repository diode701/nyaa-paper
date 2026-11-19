import QtQuick

/// The Java app's status bar: #2d2d2d, #cccccc, monospaced, 5px 15px padding.
Rectangle {
    id: bar
    property string text: ""
    implicitHeight: 26
    implicitWidth: 0
    color: "#2d2d2d"

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: "#111111"
    }

    Text {
        anchors.fill: parent
        anchors.leftMargin: 15
        anchors.rightMargin: 15
        verticalAlignment: Text.AlignVCenter
        text: bar.text
        color: "#cccccc"
        font.family: "monospace"
        font.pixelSize: 11
        elide: Text.ElideRight
    }
}
