import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

Item {
    id: view

    signal back

    property string wallpaperId: ""
    property string wallpaperTitle: ""
    property string wallpaperPreview: ""
    property string targetMonitor: Engine.monitors.length > 0 ? Engine.monitors[0] : ""

    function handleKey(event) {
        if (event.key === Qt.Key_Escape) {
            back()
            event.accepted = true
        }
    }

    property var rows: [
        { label: "Camera field of view", kind: "slider", value: 0.42 },
        { label: "Bloom", kind: "bool", value: true },
        { label: "Tint", kind: "color", value: "#5AD2FF" },
        { label: "Scene variant", kind: "combo", value: "night" },
        { label: "Background music", kind: "file", value: "~/Music/night.ogg" }
    ]

    onVisibleChanged: if (visible) list.forceActiveFocus()

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 22

        ColumnLayout {
            Layout.preferredWidth: 380
            Layout.fillHeight: true
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 214
                radius: Theme.radius
                border.color: Theme.line
                clip: true
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#1B2A33" }
                    GradientStop { position: 1.0; color: "#0C1116" }
                }
                Image {
                    anchors.fill: parent
                    anchors.margins: 6
                    source: view.wallpaperPreview
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    visible: view.wallpaperPreview !== ""
                }
                Text {
                    anchors.centerIn: parent
                    visible: view.wallpaperPreview === ""
                    text: "PREVIEW"
                    color: Theme.dim
                    font.pixelSize: 11
                    font.letterSpacing: 3
                }
            }

            Text {
                text: view.wallpaperTitle !== "" ? view.wallpaperTitle : "no selection"
                color: Theme.text
                font.pixelSize: 20
                font.bold: true
                font.letterSpacing: 1
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: "video  ·  431960  ·  engine 21:9"
                color: Theme.dim
                font.pixelSize: 11
                font.family: Theme.mono
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

            Text {
                text: "TARGET MONITOR"
                color: Theme.muted
                font.pixelSize: 10
                font.letterSpacing: 2
            }

            Row {
                spacing: 8
                Repeater {
                    model: Engine.monitors
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        width: 108
                        height: 34
                        radius: 8
                        color: modelData === view.targetMonitor ? Theme.accentDim : Theme.panelAlt
                        border.color: modelData === view.targetMonitor ? Theme.accent : Theme.line
                        Text {
                            anchors.centerIn: parent
                            text: modelData
                            color: modelData === view.targetMonitor ? Theme.accent : Theme.muted
                            font.pixelSize: 12
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: view.targetMonitor = modelData
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                Text { text: "CUSTOM"; color: Theme.accent; font.pixelSize: 11; font.letterSpacing: 3; font.bold: true }
                Text { text: "GLOBAL"; color: Theme.dim; font.pixelSize: 11; font.letterSpacing: 3 }
                Item { Layout.fillWidth: true }
                Text { text: view.rows.length + " properties"; color: Theme.dim; font.pixelSize: 11 }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 4
                height: 1
                color: Theme.line
            }

            ListView {
                id: list
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                focus: true
                model: view.rows
                spacing: 2

                delegate: Rectangle {
                    required property var modelData
                    width: list.width
                    height: 54
                    color: Theme.panel
                    radius: 6

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        Text {
                            text: modelData.label
                            color: "#FFFFFF"
                            font.pixelSize: 13
                            Layout.preferredWidth: 240
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: String(modelData.value)
                            color: Theme.muted
                            font.pixelSize: 12
                            font.family: Theme.mono
                        }
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width
                        height: 1
                        color: Theme.line
                        opacity: 0.6
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Escape) {
                        view.back()
                        event.accepted = true
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                spacing: 12
                Button {
                    text: "Apply"
                    onClicked: Engine.applyWallpaper(view.targetMonitor, view.wallpaperId)
                }
                Button {
                    text: "Stop"
                    onClicked: Engine.stop(view.targetMonitor)
                }
                Button {
                    text: "Back"
                    onClicked: view.back()
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "CTRL+ENTER apply   ·   S stop   ·   ESC back"
                    color: Theme.dim
                    font.pixelSize: 11
                }
            }
        }
    }
}
