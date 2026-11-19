import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

/// The title screen. Java: NyaaLauncher's intro (Launch / Setup / Quit),
/// grown into a menu. ↑↓ choose, Enter opens; each item also has its key.
/// Settings → Startup → Title screen turns it off; --browse skips it once.
Item {
    id: view

    signal browse
    signal playlists
    signal settings
    signal quit

    property int choice: 0
    readonly property var items: [
        { label: "Browse wallpapers", note: Library.count + " in your library", key: "ENTER", act: () => view.browse() },
        { label: "Playlists", note: Playlist.monitors.length + " saved · " + Engine.monitors.length + " monitor" + (Engine.monitors.length === 1 ? "" : "s"), key: "M", act: () => view.playlists() },
        { label: "Settings", note: "palette, paths, startup", key: "S", act: () => view.settings() },
        { label: "Quit", note: "wallpapers keep running", key: "Q", act: () => view.quit() },
    ]
    function move(delta) { choice = (choice + delta + items.length) % items.length }
    function activate() { items[choice].act() }

    readonly property int runningCount: Object.keys(Engine.running).length
    readonly property string backdropSource: {
        Controller.revision
        for (const m of Engine.monitors) {
            const p = Controller.runningPreview(m)
            if (p !== "") return p
        }
        return Library.count > 0 ? Library.at(0).preview : ""
    }

    // ---- what is running, blurred, as the room -----------------------------------
    Item {
        anchors.fill: parent
        visible: Config.backdrop
        Image {
            anchors.fill: parent
            source: view.backdropSource
            sourceSize: Qt.size(64, 64)
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            opacity: 0.30
            layer.enabled: true
            // darkened as well as faded: a bright wallpaper must not grey the window
            layer.effect: MultiEffect { blurEnabled: true; blur: 1.0; blurMax: 64; saturation: 0.1; brightness: -0.45; autoPaddingEnabled: false }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Math.max(48, parent.width * 0.09)
        anchors.rightMargin: Math.max(48, parent.width * 0.09)
        anchors.bottomMargin: 32
        spacing: 60

        // ---- brand ---------------------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 18
            Image {
                source: "qrc:/icon-128.png"
                sourceSize: Qt.size(96, 96)
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
            }
            Text {
                text: "ニャー ペーパー"
                color: Theme.dim
                font.family: "M PLUS 1 Code, Noto Sans CJK JP, Source Han Sans, " + Theme.font
                font.pixelSize: 13
                font.letterSpacing: 5
            }
            Text {
                text: "Nyaa <font color='" + Theme.accent + "'>Paper</font>"
                textFormat: Text.StyledText
                color: Theme.text
                font.family: Theme.font
                font.pixelSize: 56
                font.weight: Font.ExtraBold
                font.letterSpacing: -1
            }
            Text {
                Layout.maximumWidth: 460
                text: "Steam Workshop wallpapers on Linux. " + Library.count + " subscribed, "
                      + Engine.monitors.length + " monitor" + (Engine.monitors.length === 1 ? "" : "s") + ", "
                      + view.runningCount + " running."
                color: Theme.muted
                font.family: Theme.font
                font.pixelSize: 13
                wrapMode: Text.WordWrap
                lineHeight: 1.3
            }
        }

        // ---- menu ----------------------------------------------------------------
        ColumnLayout {
            Layout.preferredWidth: 420
            Layout.alignment: Qt.AlignVCenter
            spacing: 4
            Repeater {
                model: view.items
                delegate: Rectangle {
                    id: item
                    required property int index
                    required property var modelData
                    readonly property bool on: view.choice === index
                    Layout.fillWidth: true
                    implicitHeight: 64
                    color: on ? Qt.rgba(Theme.text.r, Theme.text.g, Theme.text.b, 0.05) : "transparent"
                    Rectangle { width: 2; height: parent.height; color: Theme.accent; visible: item.on }
                    // fixed positions, so every row's text starts at the same x
                    Text {
                        x: 18
                        anchors.verticalCenter: parent.verticalCenter
                        text: "▶"
                        color: Theme.accent
                        font.pixelSize: 11
                        opacity: item.on ? 1 : 0
                        Behavior on opacity { NumberAnimation { duration: 120 } }
                    }
                    Column {
                        x: 44
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 44 - 80
                        spacing: 2
                        Text {
                            text: item.modelData.label
                            color: item.on ? Theme.text : Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 17
                            font.weight: Font.Bold
                        }
                        Text {
                            text: item.modelData.note
                            color: item.on ? Theme.muted : Theme.dim
                            font.family: Theme.font
                            font.pixelSize: 11
                        }
                    }
                    KeyCap {
                        anchors.right: parent.right
                        anchors.rightMargin: 16
                        anchors.verticalCenter: parent.verticalCenter
                        text: item.modelData.key
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: view.choice = item.index
                        onClicked: view.activate()
                    }
                }
            }
        }
    }

    // ---- footer ------------------------------------------------------------------
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 32
        color: Theme.panel
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 14
            Text {
                text: "v" + Qt.application.version + " · " + (Engine.mock ? "mock engine" : (Engine.version !== "unknown" ? "daemon " + Engine.version : "daemon not reachable"))
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: 11
            }
            Item { Layout.fillWidth: true }
            KeyCap { text: "↑↓" }
            Text { text: "choose"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
            KeyCap { text: "ENTER" }
            Text { text: "open"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
        }
    }
}
