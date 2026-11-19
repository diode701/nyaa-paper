import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"
import "../flat"

/// PlaylistManagerView, redesigned: monitors on the left with their state,
/// numbered entries in the middle (the playing one marked), transport and
/// mode / interval / loop at the bottom. Esc/B back, ↑↓ select, Del remove,
/// Space play/pause.
Item {
    id: view

    signal back

    property string status: ""
    property int selected: -1

    function select(delta) {
        const count = Playlist.entries.length
        if (count === 0) return
        selected = Math.max(0, Math.min(count - 1, (selected < 0 ? 0 : selected + delta)))
        list.positionViewAtIndex(selected, ListView.Contain)
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
        status = ""
        action()
    }

    function clock(seconds) {
        const s = Math.round(seconds)
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0")
    }

    onVisibleChanged: {
        if (visible) {
            if (Playlist.monitor === "" && Playlist.monitors.length > 0)
                Playlist.load(Playlist.monitors[0])
            else if (Playlist.monitor !== "")
                Playlist.load(Playlist.monitor)
            status = ""
        }
    }

    readonly property bool active: Playlist.state === "playing" || Playlist.state === "paused"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- top bar ---------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.panel
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 16
                spacing: 14
                ActionButton { text: "Back"; keyHint: "ESC"; onClicked: view.back() }
                Text {
                    text: "PLAYLISTS"
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 13
                    font.weight: Font.ExtraBold
                    font.letterSpacing: 2
                }
                Item { Layout.fillWidth: true }
                Repeater {
                    model: [["↑↓", "select"], ["DEL", "remove"], ["SPACE", "play / pause"]]
                    delegate: Row {
                        required property var modelData
                        spacing: 6
                        KeyCap { text: modelData[0]; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: modelData[1]; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- monitors ------------------------------------------------------
            Rectangle {
                Layout.preferredWidth: 220
                Layout.fillHeight: true
                color: Qt.darker(Theme.panel, 1.08)
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    anchors.topMargin: 16
                    spacing: 4
                    Text {
                        text: "MONITORS"
                        color: Theme.dim
                        font.family: Theme.font
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 1.6
                        Layout.leftMargin: 8
                        Layout.bottomMargin: 6
                    }
                    Text {
                        visible: Playlist.monitors.length === 0
                        text: "No playlists yet.\nAdd entries from\nthe properties view."
                        color: Theme.dim
                        font.family: Theme.font
                        font.pixelSize: 11
                        Layout.leftMargin: 8
                    }
                    Repeater {
                        model: Playlist.monitors
                        delegate: Rectangle {
                            required property string modelData
                            readonly property bool chosen: Playlist.monitor === modelData
                            readonly property bool playing: chosen && Playlist.state === "playing"
                            readonly property bool live: Engine.running[modelData] !== undefined
                            Layout.fillWidth: true
                            implicitHeight: 38
                            radius: Theme.radius
                            color: chosen ? Theme.card : (monHover.hovered ? Qt.lighter(Theme.panel, 1.1) : "transparent")
                            HoverHandler { id: monHover }
                            Rectangle { width: 2; height: parent.height; color: Theme.accent; visible: parent.chosen }
                            Rectangle {
                                x: 12; anchors.verticalCenter: parent.verticalCenter
                                width: 7; height: 7
                                color: parent.playing ? Theme.ok : (parent.live ? Theme.accent : Theme.dim)
                            }
                            Text {
                                x: 28; anchors.verticalCenter: parent.verticalCenter
                                text: modelData
                                color: parent.chosen ? Theme.text : Theme.muted
                                font.family: Theme.font
                                font.pixelSize: 12
                                font.weight: Font.DemiBold
                            }
                            Text {
                                anchors.right: parent.right; anchors.rightMargin: 10
                                anchors.verticalCenter: parent.verticalCenter
                                text: parent.chosen ? Playlist.state : ""
                                color: Theme.dim
                                font.family: Theme.font
                                font.pixelSize: 10
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: { Playlist.load(modelData); view.selected = -1; view.status = "" }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ---- entries + transport -------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                ListView {
                    id: list
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.margins: 16
                    clip: true
                    spacing: 5
                    model: Playlist.entries
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        id: entry
                        required property int index
                        required property var modelData
                        readonly property bool now: view.active && Playlist.index === index
                        readonly property var config: modelData.config || ({})
                        width: list.width
                        height: 60
                        radius: Theme.radius
                        color: now ? Qt.tint(Theme.card, Qt.rgba(Theme.ok.r, Theme.ok.g, Theme.ok.b, 0.12))
                                   : (rowHover.hovered ? Qt.lighter(Theme.card, 1.08) : Theme.card)
                        border.color: view.selected === index ? Theme.accent : "transparent"
                        HoverHandler { id: rowHover }
                        MouseArea { anchors.fill: parent; onClicked: view.selected = index }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10
                            anchors.rightMargin: 12
                            spacing: 14
                            Text {
                                Layout.preferredWidth: 22
                                horizontalAlignment: Text.AlignRight
                                text: entry.now ? (Playlist.state === "paused" ? "❚❚" : "▶") : String(index + 1)
                                color: entry.now ? Theme.ok : Theme.dim
                                font.family: Theme.font
                                font.pixelSize: 12
                                font.weight: Font.Bold
                            }
                            Rectangle {
                                Layout.preferredWidth: 80
                                Layout.preferredHeight: 45
                                radius: Theme.radius
                                color: Theme.bg
                                clip: true
                                Image {
                                    anchors.fill: parent
                                    source: Thumbs.idFor(Library.previewFor(entry.modelData.wallpaperId))
                                    sourceSize: Qt.size(160, 90)
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true
                                    opacity: status === Image.Ready ? 1 : 0
                                    Behavior on opacity { NumberAnimation { duration: 200 } }
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    Layout.fillWidth: true
                                    text: Library.titleFor(entry.modelData.wallpaperId)
                                    color: Theme.text
                                    font.family: Theme.font
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: entry.modelData.wallpaperId
                                          + (entry.config.gpuMode && entry.config.gpuMode !== "DEFAULT"
                                             ? " · " + entry.config.gpuMode + (entry.config.gpuMode === "DRI_PRIME" ? " " + entry.config.driPrimeNumber : "")
                                             : "")
                                    color: Theme.dim
                                    font.family: Theme.font
                                    font.pixelSize: 10
                                    elide: Text.ElideRight
                                }
                            }
                            Text {
                                text: entry.modelData.durationSec > 0.5
                                      ? view.clock(entry.modelData.durationSec) + " video"
                                      : view.clock(Playlist.interval)
                                color: Theme.muted
                                font.family: Theme.font
                                font.pixelSize: 11
                            }
                            Column {
                                spacing: 2
                                Repeater {
                                    model: [["▲", -1], ["▼", 1]]
                                    delegate: Text {
                                        required property var modelData
                                        text: modelData[0]
                                        color: arrowHover.hovered ? Theme.text : Theme.dim
                                        font.pixelSize: 9
                                        HoverHandler { id: arrowHover }
                                        MouseArea { anchors.fill: parent; anchors.margins: -3; onClicked: Playlist.move(entry.index, entry.index + modelData[1]) }
                                    }
                                }
                            }
                            Text {
                                text: "✕"
                                color: Theme.danger
                                opacity: rowHover.hovered ? 1 : 0.5
                                font.pixelSize: 12
                                MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: Playlist.removeAt(entry.index) }
                            }
                        }
                    }

                    Text {
                        anchors.centerIn: parent
                        visible: Playlist.entries.length === 0
                        text: Playlist.monitor === "" ? "Choose a monitor on the left."
                                                      : "No entries on " + Playlist.monitor + ".\nOpen a wallpaper and press A to add it."
                        color: Theme.dim
                        font.family: Theme.font
                        font.pixelSize: 12
                        horizontalAlignment: Text.AlignHCenter
                        lineHeight: 1.4
                    }
                }

                // ---- transport ---------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: Theme.panel
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 16
                        anchors.rightMargin: 16
                        spacing: 6

                        component Transport: ActionButton {
                            implicitWidth: 40
                            font.pixelSize: 13
                        }
                        Transport { text: "↺"; onClicked: view.onMonitor(Playlist.reset) }
                        Transport { text: "⏮"; onClicked: view.onMonitor(Playlist.prev) }
                        Transport {
                            implicitWidth: 54
                            text: Playlist.state === "playing" ? "❚❚" : "▶"
                            color: Theme.accent
                            onClicked: view.onMonitor(Playlist.state === "idle" ? Playlist.play : Playlist.togglePause)
                        }
                        Transport { text: "⏭"; onClicked: view.onMonitor(Playlist.next) }
                        Transport { text: "■"; outline: true; color: Theme.danger; onClicked: view.onMonitor(Playlist.stop) }

                        Text {
                            Layout.fillWidth: true
                            Layout.leftMargin: 10
                            text: view.status !== "" ? view.status
                                  : Playlist.message !== "" ? Playlist.message
                                  : (view.active ? Playlist.state + " · entry " + (Playlist.index + 1) + " of " + Playlist.entries.length : "")
                            color: view.status !== "" ? Theme.warn : Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }

                        Segmented {
                            options: ["SEQUENTIAL", "RANDOM"]
                            value: Playlist.mode
                            onPicked: v => view.onMonitor(() => Playlist.setMode(v))
                        }
                        Stepper {
                            implicitWidth: 104
                            from: 5; to: 3600; step: 5; suffix: " s"
                            value: Playlist.interval
                            onModified: v => view.onMonitor(() => Playlist.setInterval(v))
                        }
                        Toggle {
                            text: "Loop"
                            checked: Playlist.loop
                            onToggled: c => view.onMonitor(() => Playlist.setLoop(c))
                        }
                    }
                }
            }
        }
    }
}
