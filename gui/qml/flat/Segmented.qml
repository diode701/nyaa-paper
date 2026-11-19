import QtQuick
import QtQuick.Layouts
import NyaaPaper.Theme 1.0

/// A few choices side by side; the chosen one is outlined in the accent.
Row {
    id: seg
    property var options: []        // strings
    property string value: ""
    property bool upper: true
    signal picked(string value)
    spacing: -1

    Repeater {
        model: seg.options
        delegate: Rectangle {
            required property string modelData
            readonly property bool on: modelData === seg.value
            width: label.implicitWidth + 22
            height: 26
            radius: Theme.radius
            color: on ? Theme.raise : "transparent"
            border.color: on ? Theme.accent : Theme.line
            z: on ? 1 : 0
            Text {
                id: label
                anchors.centerIn: parent
                text: seg.upper ? modelData.toUpperCase() : modelData
                color: parent.on ? Theme.text : Theme.dim
                font.family: Theme.font
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: seg.upper ? 0.8 : 0
            }
            MouseArea { anchors.fill: parent; onClicked: seg.picked(modelData) }
        }
    }
}
