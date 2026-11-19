import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: view

    signal back

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 22

        ColumnLayout {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            spacing: 8

            Text {
                text: "MONITORS"
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 2
            }

            Repeater {
                model: Engine.monitors
                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    Layout.fillWidth: true
                    height: 46
                    radius: 8
                    color: index === 0 ? Theme.panelAlt : "transparent"
                    border.color: index === 0 ? Theme.accent : Theme.line

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        text: modelData
                        color: Theme.text
                        font.pixelSize: 12
                    }
                    Rectangle {
                        width: 6
                        height: 6
                        radius: 3
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        color: Engine.running[modelData] !== undefined ? Theme.accent : Theme.dim
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            Text {
                text: "PLAYLIST"
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 2
            }

            Repeater {
                model: 4
                delegate: Rectangle {
                    required property int index
                    Layout.fillWidth: true
                    height: 64
                    radius: 8
                    color: index === 0 ? Theme.panelAlt : "transparent"
                    border.color: index === 0 ? Theme.accent : Theme.line

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 14

                        Text {
                            text: ("0" + (index + 1)).slice(-2)
                            color: Theme.dim
                            font.family: Theme.mono
                            font.pixelSize: 11
                        }
                        Rectangle {
                            width: 64
                            height: 36
                            radius: 4
                            color: Theme.accentDim
                        }
                        ColumnLayout {
                            spacing: 2
                            Text { text: "wallpaper " + (index + 1); color: Theme.text; font.pixelSize: 13 }
                            Text { text: "42s"; color: Theme.dim; font.pixelSize: 10; font.family: Theme.mono }
                        }
                        Item { Layout.fillWidth: true }
                        Text { text: "ALT+\u2191/\u2193 reorder"; color: Theme.dim; font.pixelSize: 10 }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                Text { text: "SEQUENTIAL"; color: Theme.accent; font.pixelSize: 10; font.letterSpacing: 2 }
                Text { text: "every 60s"; color: Theme.muted; font.pixelSize: 11 }
                Text { text: "loop on"; color: Theme.muted; font.pixelSize: 11 }
                Item { Layout.fillWidth: true }
                Text {
                    text: "\u25B6 play    \u23F8 pause    \u23F9 stop    \u23EE prev    \u23ED next"
                    color: Theme.muted
                    font.pixelSize: 12
                }
            }
        }
    }
}
