import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
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
    font.family: Theme.font

    Material.theme: Material.Dark
    Material.accent: Theme.accent
    Material.background: Theme.panel
    Material.foreground: Theme.text
    Material.roundedScale: Material.NotRounded

    property int view: 0
    property bool settingsOpen: false
    property bool keysOpen: false
    /// Hidden in the tray: the grid lets go of its thumbnails (suspendToTray)
    property bool suspended: false

    /// WallpaperBrowser.collapseToTray / suspendToTray
    function hideToTray() {
        suspended = true
        app.hide()
        app.releaseResources()
    }

    /// restoreFromTray
    function restoreFromTray() {
        suspended = false
        app.show()
        app.raise()
        app.requestActivate()
        root.forceActiveFocus()
    }

    // Java: closing the window collapses to the tray instead of quitting.
    onClosing: (close) => {
        if (Tray.available) {
            close.accepted = false
            hideToTray()
        }
    }
    property bool startGlobal: false
    property bool startMini: false
    property string currentId: ""
    property string currentTitle: ""
    property string currentPreview: ""
    property string currentBasePath: ""

    // read by the NYAA_KEYS dev hook; no effect on behaviour
    readonly property int debugCursor: browser.cursor
    readonly property int debugPage: browser.page

    // Window-scoped shortcuts: they do not care which item holds focus.
    // single-key shortcuts are off while typing: search, or the Settings dialog
    readonly property bool keysEnabled: !app.settingsOpen && (app.view !== 0 || !browser.searchOpen)

    Shortcut { sequence: "Ctrl+Q"; onActivated: Qt.quit() }
    Shortcut { sequence: "Q";       enabled: app.keysEnabled; onActivated: Qt.quit() }

    Shortcut { sequence: "Left";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(-1, 0) }
    Shortcut { sequence: "Right";  enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(1, 0) }
    Shortcut { sequence: "Up";     enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(0, -1) }
    Shortcut { sequence: "Down";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.move(0, 1) }
    Shortcut { sequence: "PgUp";   enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page - 1) }
    Shortcut { sequence: "PgDown"; enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page + 1) }
    Shortcut { sequence: "Return"; enabled: app.keysEnabled && app.view === 0; onActivated: browser.openCurrent() }
    Shortcut { sequence: "Enter";  enabled: app.keysEnabled && app.view === 0; onActivated: browser.openCurrent() }
    Shortcut { sequence: "/";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.openSearch() }
    // WallpaperBrowser: PAGE_UP / N = previous page, PAGE_DOWN / P = next page
    Shortcut { sequence: "N";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page - 1) }
    Shortcut { sequence: "P";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.goTo(browser.page + 1) }
    Shortcut { sequence: "M";      enabled: app.keysEnabled && app.view === 0; onActivated: browser.playlists() }
    Shortcut { sequence: "G";      enabled: app.keysEnabled && app.view === 0; onActivated: Settings.cycle() }
    Shortcut { sequences: ["?", "H"]; enabled: app.keysEnabled && app.view === 0; onActivated: app.keysOpen = !app.keysOpen }
    Shortcut { sequence: "S";      enabled: app.keysEnabled && app.view === 0; onActivated: app.settingsOpen = true }
    Shortcut { sequence: "Escape"; enabled: app.view !== 0 && app.view !== 4; onActivated: { app.view = 0; root.forceActiveFocus() } }
    // the browser's Esc goes back to the title screen (when it is on)
    Shortcut { sequence: "Escape"; enabled: app.view === 0 && app.keysEnabled && Config.titleScreen; onActivated: app.view = 4 }
    // title screen: ↑↓ choose, Enter opens, or each item's key
    Shortcut { sequence: "Up"; enabled: app.view === 4 && app.keysEnabled; onActivated: title.move(-1) }
    Shortcut { sequence: "Down"; enabled: app.view === 4 && app.keysEnabled; onActivated: title.move(1) }
    Shortcut { sequences: ["Return", "Enter"]; enabled: app.view === 4 && app.keysEnabled; onActivated: title.activate() }
    Shortcut { sequence: "M"; enabled: app.view === 4 && app.keysEnabled; onActivated: app.view = 2 }
    Shortcut { sequence: "S"; enabled: app.view === 4 && app.keysEnabled; onActivated: app.settingsOpen = true }
    Shortcut {
        sequence: "Ctrl+Return"
        enabled: app.view === 1
        onActivated: properties.applyCurrent()
    }
    Shortcut {
        sequence: "Ctrl+Up"
        enabled: app.view === 1
        onActivated: properties.cycleMonitor(-1)
    }
    Shortcut {
        sequence: "Ctrl+Down"
        enabled: app.view === 1
        onActivated: properties.cycleMonitor(1)
    }
    Shortcut {
        sequence: "L"
        enabled: app.view === 1 && !properties.miniOpen
        onActivated: properties.loadLast()
    }
    // Wallpaper / Engine tabs: [ and ] (Ctrl+Tab belongs to the desktop)
    Shortcut { sequence: "["; enabled: app.view === 1; onActivated: properties.showGlobal = false }
    Shortcut { sequence: "]"; enabled: app.view === 1; onActivated: properties.showGlobal = true }
    Shortcut {
        sequence: "Ctrl+L"
        enabled: app.view === 1
        onActivated: properties.toggleMini()
    }
    Shortcut {
        sequence: "N"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: Playlist.next()
    }
    Shortcut {
        sequence: "P"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: Playlist.prev()
    }
    // MiniPlaylistPanel.setupKeyboard, only while the panel is open
    Shortcut {
        sequence: "Up"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: properties.miniSelect(-1)
    }
    Shortcut {
        sequence: "Down"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: properties.miniSelect(1)
    }
    Shortcut {
        sequence: "Delete"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: properties.miniRemove(properties.miniSelected)
    }
    Shortcut {
        sequence: "Space"
        enabled: app.view === 1 && properties.miniOpen
        onActivated: Playlist.state === "playing" ? Playlist.pause() : Playlist.play()
    }
    Shortcut {
        sequence: "B"
        enabled: app.view === 2
        onActivated: { app.view = 0; root.forceActiveFocus() }
    }
    Shortcut {
        sequence: "Up"
        enabled: app.view === 2
        onActivated: playlistView.select(-1)
    }
    Shortcut {
        sequence: "Down"
        enabled: app.view === 2
        onActivated: playlistView.select(1)
    }
    Shortcut {
        sequence: "Delete"
        enabled: app.view === 2
        onActivated: playlistView.removeSelected()
    }
    Shortcut {
        sequence: "1"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(0)
    }
    Shortcut {
        sequence: "2"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(1)
    }
    Shortcut {
        sequence: "3"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(2)
    }
    Shortcut {
        sequence: "4"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(3)
    }
    Shortcut {
        sequence: "5"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(4)
    }
    Shortcut {
        sequence: "6"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(5)
    }
    Shortcut {
        sequence: "7"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(6)
    }
    Shortcut {
        sequence: "8"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(7)
    }
    Shortcut {
        sequence: "9"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(8)
    }
    Shortcut {
        sequence: "0"
        enabled: app.view === 1
        onActivated: properties.selectMonitorByIndex(9)
    }
    Shortcut {
        sequence: "S"
        enabled: app.view === 1 && app.keysEnabled
        onActivated: properties.stopCurrent()
    }
    Shortcut {
        sequence: "A"
        enabled: app.view === 1 && app.keysEnabled
        onActivated: properties.addCurrent()
    }
    Shortcut {
        sequence: "Space"
        enabled: app.view === 2
        onActivated: Playlist.state === "playing" ? Playlist.pause() : Playlist.play()
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
                    suspended: app.suspended
                    visible: app.view === 0
                    onOpen: (wallpaperId, title, preview, basePath) => {
                        app.currentId = wallpaperId
                        app.currentTitle = title
                        app.currentPreview = preview
                        app.currentBasePath = basePath
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
                    wallpaperBasePath: app.currentBasePath
                    startGlobal: app.startGlobal
                    startMini: app.startMini
                    anchors.fill: parent
                    visible: app.view === 1
                    onBack: { app.view = 0; root.forceActiveFocus() }
                }
                PlaylistView {
                    id: playlistView
                    anchors.fill: parent
                    visible: app.view === 2
                    onBack: { app.view = 0; root.forceActiveFocus() }
                }
                TitleView {
                    id: title
                    anchors.fill: parent
                    visible: app.view === 4
                    onBrowse: { app.view = 0; root.forceActiveFocus() }
                    onPlaylists: { app.view = 2; root.forceActiveFocus() }
                    onSettings: app.settingsOpen = true
                    onQuit: Qt.quit()
                }
            }

        }

        StatusBar {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            visible: app.view === 0
            pageLabel: browser.pageLabel
            selection: browser.cursorTitle
            cols: browser.cols
            rows: browser.rows
        }
        KeysOverlay {
            id: keysOverlay
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 14
            anchors.bottomMargin: 42
            open: app.view === 0 && app.keysOpen
        }

        SettingsView {
            id: settingsView
            anchors.fill: parent
            open: app.settingsOpen
            onClose: {
                app.settingsOpen = false
                root.forceActiveFocus()
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
        target: Tray
        function onRestoreRequested() { app.restoreFromTray() }
    }
    Connections {
        target: Config
        function onProblem(message) { toast.show(message) }
    }

    Connections {
        target: Engine
        function onError(message) { toast.show(message) }
        function onEngineExited(monitor, code) {
            toast.show("engine stopped on " + monitor + " (code " + code + ")")
            Engine.refreshRunning()
        }
    }

    /// Keyboard focus belongs to the open overlay, else to the window's keys.
    function refocus() {
        if (settingsOpen) settingsView.forceActiveFocus()
        else root.forceActiveFocus()
    }
    onActiveChanged: if (active) refocus()
    onSettingsOpenChanged: refocus()

    Component.onCompleted: {
        // Java's launcher intro; Settings → Startup turns it off, --browse skips it once
        if (Config.titleScreen && Qt.application.arguments.indexOf("--browse") < 0)
            app.view = 4
        root.forceActiveFocus()
        Engine.refreshRunning()
    }
}
