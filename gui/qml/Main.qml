import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "views"
import "components"

ApplicationWindow {
    id: app
    width: 1280
    height: 800
    minimumWidth: 780
    minimumHeight: 520
    visible: true
    title: "Nyaa Paper"
    color: Theme.bg

    property int view: 0
    property string currentId: ""
    property string currentTitle: ""
    property string currentPreview: ""

    // read by the NYAA_KEYS dev hook; no effect on behaviour
    readonly property int debugCursor: browser.cursor
    readonly property int debugPage: browser.page

    // Window-scoped shortcuts: they do not care which item holds focus.
    readonly property bool keysEnabled: app.view !== 0 || !browser.searchOpen

    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }

    Shortcut { sequence: "Left";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(-1, 0) }
    Shortcut { sequence: "Right";  enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(1, 0) }
    Shortcut { sequence: "Up";     enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(0, -1) }
    Shortcut { sequence: "Down";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(0, 1) }
    Shortcut { sequence: "PgUp";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page - 1, 0) }
    Shortcut { sequence: "PgDown"; enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page + 1, 0) }
    Shortcut { sequence: "Return"; enabled: app.keysEnabled && app.view === 0; onActivated: browser.openCurrent() }
    Shortcut { sequence: "Enter";  enabled: app.keysEnabled && app.view === 0; onActivated: browser.openCurrent() }
    Shortcut { sequence: "/";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.openSearch() }
    Shortcut { sequence: "M";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.playlists() }
    Shortcut { sequence: "G";      enabled: app.keysEnabled && app.view === 0; onActivated: Settings.cycle() }
    Shortcut { sequence: "Escape"; enabled: app.view !== 0; onActivated: { app.view = 0; root.forceActiveFocus() } }
    Shortcut {
        sequence: "Ctrl+Return"
        enabled: app.view === 1
        onActivated: Engine.applyWallpaper(properties.targetMonitor, properties.wallpaperId)
    }
    Shortcut {
        sequence: "S"
        enabled: app.view === 1 && app.keysEnabled
        onActivated: Engine.stop(properties.targetMonitor)
    }
    Shortcut {
        sequence: "A"
        enabled: app.view === 0 && app.keysEnabled
        onActivated: Engine.applyWallpaper(Engine.monitors.length > 0 ? Engine.monitors[0] : "", app.currentId)
    }

    Item {
        id: root
        anchors.fill: parent
        focus: true

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                BrowserView {
                    id: browser
                    anchors.fill: parent
                    visible: app.view === 0
                    onOpen: (wallpaperId, title, preview) => {
                        app.currentId = wallpaperId
                        app.currentTitle = title
                        app.currentPreview = preview
                        app.view = 1
                        root.forceActiveFocus()
                    }
                    onPlaylists: { app.view = 2; root.forceActiveFocus() }
                    onRequestFocus: root.forceActiveFocus()
                }
                PropertiesView {
                    id: properties
                    wallpaperId: app.currentId
                    wallpaperTitle: app.currentTitle
                    wallpaperPreview: app.currentPreview
                    anchors.fill: parent
                    visible: app.view === 1
                    onBack: { app.view = 0; root.forceActiveFocus() }
                }
                PlaylistView {
                    id: playlist
                    anchors.fill: parent
                    visible: app.view === 2
                    onBack: { app.view = 0; root.forceActiveFocus() }
                }
            }

            HintBar {
                Layout.fillWidth: true
                mode: app.view
                pageLabel: app.view === 0 ? browser.pageLabel : ""
                selection: app.view === 0 ? browser.cursorTitle : ""
            }
        }

        Toast {
            id: toast
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 18
        }
    }

    Connections {
        target: Engine
        function onError(message) { toast.show(message) }
        function onEngineExited(monitor, code) {
            toast.show("engine stopped on " + monitor + " (code " + code + ")")
            Engine.refreshRunning()
        }
    }

    onActiveChanged: if (active) root.forceActiveFocus()

    Component.onCompleted: {
        root.forceActiveFocus()
        Engine.refreshRunning()
    }
}
