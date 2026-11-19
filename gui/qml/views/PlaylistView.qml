import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: view

    signal back

    function handleKey(event) {
        if (event.key === Qt.Key_Escape) {
            back()
            event.accepted = true
        }
    }

    onVisibleChanged: {
        if (visible)
            Playlist.load(Engine.monitors.length > 0 ? Engine.monitors[0] : "")
    }

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
                    color: Playlist.monitor === modelData ? Theme.panelAlt : "transparent"
                    border.color: Playlist.monitor === modelData ? Theme.accent : Theme.line

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
                    MouseArea {
                        anchors.fill: parent
                        onClicked: Playlist.load(modelData)
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "PLAYLIST"
                    color: Theme.muted
                    font.pixelSize: 10
                    font.letterSpacing: 2
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: Playlist.state + (Playlist.index >= 0 ? "  ·  " + (Playlist.index + 1) : "")
                    color: Theme.accent
                    font.pixelSize: 10
                    font.letterSpacing: 2
                }
            }

            Text {
                visible: Playlist.entries.length === 0
                text: "no entries yet — press A in the properties view to add the selected wallpaper"
                color: Theme.dim
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            Repeater {
                model: Playlist.entries
                delegate: Rectangle {
                    required property int index
                    required property var modelData
                    Layout.fillWidth: true
                    height: 52
                    radius: 8
                    color: Playlist.index === index ? Theme.panelAlt : "transparent"
                    border.color: Playlist.index === index ? Theme.accent : Theme.line

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
                        Text {
                            text: "wallpaper " + modelData.wallpaperId
                            color: Theme.text
                            font.pixelSize: 13
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.durationSec > 0
                                  ? Math.round(modelData.durationSec) + "s"
                                  : "interval"
                            color: Theme.dim
                            font.pixelSize: 10
                            font.family: Theme.mono
                        }
                        Button {
                            text: "✕"
                            onClicked: Playlist.removeAt(index)
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Button {
                    text: "▶ Play"
                    onClicked: Playlist.play()
                }
                Button {
                    text: "⏸ Pause"
                    onClicked: Playlist.pause()
                }
                Button {
                    text: "⏹ Stop"
                    onClicked: Playlist.stop()
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "space play/pause   ·   esc back"
                    color: Theme.dim
                    font.pixelSize: 10
                }
            }
        }
    }
}
