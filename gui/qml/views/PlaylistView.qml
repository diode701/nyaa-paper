import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

/// A 1:1 port of PlaylistManagerView: top bar, 180px monitor column, entry rows
/// with thumbnails and reorder buttons, bottom control bar with mode/interval/loop.
Item {
    id: view

    signal back

    property string status: "Select a monitor"
    property int selected: -1

    function select(delta) {
        const count = Playlist.entries.length
        if (count === 0) return
        selected = Math.max(0, Math.min(count - 1, (selected < 0 ? 0 : selected + delta)))
    }

    function removeSelected() {
        if (selected >= 0 && selected < Playlist.entries.length) {
            Playlist.removeAt(selected)
            selected = Math.min(selected, Playlist.entries.length - 1)
        }
    }

    // Java runOnMonitor: every control needs a selected monitor first.
    function onMonitor(action) {
        if (Playlist.monitor === "") { status = "Select a monitor first"; return }
        action()
    }

    onVisibleChanged: {
        if (visible) {
            if (Playlist.monitor !== "") Playlist.load(Playlist.monitor)
            status = Playlist.monitor === "" ? "Select a monitor" : ""
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#2d2d2d"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10
                ActionButton {
                    text: "← Back"
                    onClicked: view.back()
                }
                Text {
                    text: "Playlist Manager" + (Playlist.monitor !== "" ? " — " + Playlist.monitor : "")
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "Esc/B: back | ↑↓: navigate | Del: remove | Space: play/pause"
                    color: "#aaaaaa"
                    font.pixelSize: 10
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 10
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 180
                Layout.fillHeight: true
                color: "#2a2a2a"
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 5
                    spacing: 5
                    Text {
                        visible: Playlist.monitors.length === 0
                        text: "No playlists yet.\nAdd entries from\nProperties view."
                        color: "#808080"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.margins: 10
                    }
                    Repeater {
                        model: Playlist.monitors
                        delegate: ActionButton {
                            required property string modelData
                            Layout.fillWidth: true
                            text: modelData
                            bold: Playlist.monitor === modelData
                            color: "#3a3a3a"
                            onClicked: {
                                Playlist.load(modelData)
                                view.selected = -1
                                view.status = ""
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: parent.width
                    spacing: 5

                    Repeater {
                        model: Playlist.entries
                        delegate: Rectangle {
                            required property int index
                            required property var modelData

                            Layout.fillWidth: true
                            height: 57
                            radius: 4
                            color: view.selected === index ? "#333333" : "#2a2a2a"
                            border.width: view.selected === index ? 2 : 1
                            border.color: view.selected === index ? Theme.accent : "#3a3a3a"

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 10

                                Image {
                                    Layout.preferredWidth: 80
                                    Layout.preferredHeight: 45
                                    source: Library.previewFor(modelData.wallpaperId)
                                    fillMode: Image.PreserveAspectFit
                                    asynchronous: true
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: Library.titleFor(modelData.wallpaperId)
                                    color: "#d3d3d3"
                                    font.pixelSize: 12
                                    wrapMode: Text.WordWrap
                                    maximumLineCount: 2
                                    elide: Text.ElideRight
                                }
                                Column {
                                    spacing: 0
                                    ActionButton {
                                        text: "↑"
                                        implicitWidth: 22
                                        implicitHeight: 20
                                        onClicked: Playlist.move(index, index - 1)
                                    }
                                    ActionButton {
                                        text: "↓"
                                        implicitWidth: 22
                                        implicitHeight: 20
                                        onClicked: Playlist.move(index, index + 1)
                                    }
                                }
                                Text {
                                    text: "✕"
                                    color: "#d9534f"
                                    font.pixelSize: 12
                                    MouseArea {
                                        anchors.fill: parent
                                        onClicked: Playlist.removeAt(index)
                                    }
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                z: -1
                                onClicked: view.selected = index
                            }
                        }
                    }

                    Text {
                        visible: Playlist.entries.length === 0
                        text: "No entries in playlist"
                        color: "#aaaaaa"
                        font.pixelSize: 11
                        Layout.fillWidth: true
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 56
            color: "#2d2d2d"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                ActionButton { text: "▶ Play"; color: Theme.accent; onClicked: view.onMonitor(Playlist.play) }
                ActionButton { text: "⏸ Pause"; color: Theme.warning; onClicked: view.onMonitor(Playlist.togglePause) }
                ActionButton { text: "⏹ Stop"; color: Theme.danger; onClicked: view.onMonitor(Playlist.stop) }
                Rectangle { width: 1; height: 24; color: "#3a3a3a" }
                ActionButton { text: "↺ Reset"; color: "#999999"; onClicked: view.onMonitor(Playlist.reset) }
                ActionButton { text: "⏮ Prev"; color: "#888888"; onClicked: view.onMonitor(Playlist.prev) }
                ActionButton { text: "Next ⏭"; color: "#888888"; onClicked: view.onMonitor(Playlist.next) }

                Item { Layout.fillWidth: true }

                Text { text: "Mode:"; color: "#d3d3d3"; font.pixelSize: 12 }
                ComboBox {
                    Layout.preferredWidth: 130
                    implicitHeight: 28
                    font.pixelSize: 12
                    model: ["SEQUENTIAL", "RANDOM"]
                    currentIndex: Playlist.mode === "RANDOM" ? 1 : 0
                    onActivated: Playlist.setMode(currentText)
                }
                Text { text: "Interval (s):"; color: "#d3d3d3"; font.pixelSize: 12 }
                SpinBox {
                    Layout.preferredWidth: 90
                    from: 5
                    to: 3600
                    value: Math.max(5, Playlist.interval)
                    onValueModified: Playlist.setInterval(value)
                }
                CheckBox {
                    text: "Loop"
                    checked: Playlist.loop
                    onToggled: Playlist.setLoop(checked)
                }
                Text { text: view.status; color: "#cccccc"; font.pixelSize: 11 }
            }
        }
    }
}
