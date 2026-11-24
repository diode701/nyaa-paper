import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import Qt.labs.qmlmodels
import "../components"
import "../controls"
import "../flat"

/// WallpaperPropertiesView, redesigned: the wallpaper and where it goes on the
/// left, its settings on the right in two tabs (Wallpaper: its own
/// properties; Engine: this monitor's linux-wallpaperengine options, GPU,
/// capture and theme), actions at the bottom. [ ] switch tabs.
Item {
    id: view

    signal back

    property string wallpaperId: ""
    property string wallpaperTitle: ""
    property string wallpaperPreview: ""
    property string wallpaperBasePath: ""
    property string targetMonitor: Engine.monitors.length > 0 ? Engine.monitors[0] : ""
    property string status: "Ready"
    property bool showGlobal: false
    property bool startGlobal: false
    property bool miniOpen: false
    property bool startMini: false
    onStartGlobalChanged: if (startGlobal) showGlobal = true
    onStartMiniChanged: if (startMini) toggleMini()

    function toggleMini() { miniOpen = !miniOpen }

    // MiniPlaylistPanel.show() refreshes, however it was opened (button,
    // Ctrl+L or the hover strip), and setMonitor() follows the combo.
    property int miniSelected: -1
    onMiniOpenChanged: if (miniOpen) { miniSelected = -1; Playlist.load(targetMonitor) }
    onTargetMonitorChanged: if (miniOpen) { miniSelected = -1; Playlist.load(targetMonitor) }

    /// MiniPlaylistPanel.setupKeyboard: ↑/↓ select, Del remove
    function miniSelect(delta) {
        const count = Playlist.entries.length
        if (count === 0) return
        miniSelected = Math.max(0, Math.min(count - 1, miniSelected < 0 ? 0 : miniSelected + delta))
    }
    function miniRemove(row) {
        if (row < 0 || row >= Playlist.entries.length) return
        Playlist.removeAt(row)
        miniSelected = -1
    }

    // The logic lives in PropertiesController (Java: WallpaperPropertiesView's
    // applyToMonitor / addToPlaylist / loadSavedSettings / stopOnMonitor).
    function applyCurrent() { Controller.apply(targetMonitor) }
    function loadLast() { Controller.loadLast(targetMonitor) }
    function stopCurrent() { Controller.stop(targetMonitor) }

    function addCurrent() {
        if (targetMonitor === "") { status = "Please select a monitor first."; return }
        // Java addToPlaylist: load THAT monitor's playlist, append, save
        const count = Playlist.addTo(targetMonitor, Controller.playlistEntry(targetMonitor))
        if (count < 0) { status = "Could not add to the playlist (is the daemon running?)"; return }
        status = "Added to " + targetMonitor + " playlist (" + count + " entries)"
        if (miniOpen) Playlist.load(targetMonitor)
    }

    function openWallpaper() {
        const count = Controller.open(wallpaperId, wallpaperBasePath)
        status = count > 0 ? "Loaded " + count + " custom properties" : "Ready"
        // Opening the wallpaper that is running here: show what it runs with
        if (Engine.running[targetMonitor] !== undefined
                && Controller.runningId(targetMonitor) === wallpaperId)
            loadLast()
    }

    Connections {
        target: Controller
        function onStatus(message) { view.status = message }
    }

    function cycleMonitor(delta) {
        const monitors = Engine.monitors
        if (monitors.length === 0) return
        const current = monitors.indexOf(targetMonitor)
        const next = (current < 0 ? 0 : (current + delta + monitors.length) % monitors.length)
        targetMonitor = monitors[next]
    }

    function selectMonitorByIndex(index) {
        const monitors = Engine.monitors
        if (index >= 0 && index < monitors.length)
            targetMonitor = monitors[index]
    }

    onVisibleChanged: if (visible) openWallpaper()
    onWallpaperBasePathChanged: if (visible) openWallpaper()

    component Section: Text {
        Layout.fillWidth: true
        Layout.leftMargin: 22
        Layout.topMargin: 12
        color: Theme.accent
        font.family: Theme.font
        font.pixelSize: 11
        font.weight: Font.Bold
        font.letterSpacing: 1.4
    }
    component Row2: SettingCard {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- top bar: back and the wallpaper's actions --------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: Theme.panel
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 8
                ActionButton { text: "Back"; keyHint: "ESC"; onClicked: view.back() }
                Item { Layout.fillWidth: true }
                ActionButton { text: "Load last"; keyHint: "L"; onClicked: view.loadLast() }
                ActionButton { text: "Add to playlist"; keyHint: "A"; outline: true; color: Theme.ok; onClicked: view.addCurrent() }
                ActionButton { text: "Playlist"; keyHint: "⌃L"; onClicked: view.toggleMini() }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- left: the wallpaper and where it goes --------------------
            Rectangle {
                Layout.preferredWidth: 340
                Layout.fillHeight: true
                color: Qt.darker(Theme.panel, 1.08)
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 12
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: width * 10 / 16
                        radius: Theme.radius
                        color: Theme.card
                        border.color: Theme.line
                        clip: true
                        AnimatedImage {
                            anchors.fill: parent
                            anchors.margins: 1
                            source: view.wallpaperPreview
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            playing: Settings.animationMode !== 0 && view.visible
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: view.wallpaperTitle
                        color: Theme.text
                        font.family: Theme.font
                        font.pixelSize: 15
                        font.weight: Font.Bold
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                    GridLayout {
                        columns: 2
                        columnSpacing: 14
                        rowSpacing: 4
                        Text { text: "TYPE"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1 }
                        Text { text: Library.at(Library.indexOf(view.wallpaperId)).kind || "—"; color: Theme.muted; font.family: Theme.font; font.pixelSize: 12 }
                        Text { text: "ID"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1 }
                        Text { text: view.wallpaperId; color: Theme.muted; font.family: Theme.font; font.pixelSize: 12 }
                    }
                    Item { Layout.preferredHeight: 4 }
                    Text {
                        text: "APPLY TO · 1–9 OR CLICK"
                        color: Theme.dim
                        font.family: Theme.font
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 1.4
                    }
                    MonitorMap {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 118
                        selected: view.targetMonitor
                        revision: Controller.revision
                        onPicked: m => view.targetMonitor = m
                    }
                    Item { Layout.fillHeight: true }
                }
            }

            // ---- right: tabs and settings ----------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: Theme.bg
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.line }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 4
                        Repeater {
                            model: [{ label: "WALLPAPER", count: String(Properties.count), engine: false },
                                    { label: "ENGINE", count: view.targetMonitor, engine: true }]
                            delegate: Item {
                                required property var modelData
                                readonly property bool on: modelData.engine === view.showGlobal
                                Layout.fillHeight: true
                                implicitWidth: tabRow.implicitWidth + 24
                                Row {
                                    id: tabRow
                                    anchors.centerIn: parent
                                    spacing: 8
                                    Text { text: modelData.label; color: parent.parent.on ? Theme.text : Theme.dim; font.family: Theme.font; font.pixelSize: 12; font.weight: Font.Bold; font.letterSpacing: 1 }
                                    Rectangle {
                                        width: n.implicitWidth + 10; height: 17; radius: Theme.radius; color: Theme.panel
                                        anchors.verticalCenter: parent.verticalCenter
                                        Text { id: n; anchors.centerIn: parent; text: modelData.count; color: Theme.dim; font.family: Theme.font; font.pixelSize: 10 }
                                    }
                                }
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 2; color: Theme.accent; visible: parent.on }
                                MouseArea { anchors.fill: parent; onClicked: view.showGlobal = modelData.engine }
                            }
                        }
                        Item { Layout.fillWidth: true }
                        KeyCap { text: "[" }
                        KeyCap { text: "]" }
                        Text { text: "switch"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                    }
                }

                // ---- Wallpaper tab: the Setting classes, one control each --
                ScrollView {
                    id: wallpaperScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: !view.showGlobal
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ColumnLayout {
                        width: wallpaperScroll.availableWidth
                        spacing: 6
                        Item { Layout.preferredHeight: 8 }
                        // Java: for each Setting, box.add(setting.createControl(...))
                        Repeater {
                            model: Properties
                            delegate: DelegateChooser {
                                role: "kind"
                                DelegateChoice { roleValue: 0; BoolControl {} }
                                DelegateChoice { roleValue: 1; SliderControl {} }
                                DelegateChoice { roleValue: 2; ColorControl {} }
                                DelegateChoice { roleValue: 3; ComboControl {} }
                                DelegateChoice { roleValue: 4; TextControl {} }
                                DelegateChoice { roleValue: 5; GroupControl {} }
                                DelegateChoice { roleValue: 6; TextInputControl {} }
                                DelegateChoice { roleValue: 7; FileControl {} }
                                DelegateChoice { roleValue: 8; DirectoryControl {} }
                                DelegateChoice { roleValue: 9; TextureControl {} }
                            }
                        }
                        Text {
                            visible: Properties.count === 0
                            Layout.leftMargin: 22
                            Layout.topMargin: 10
                            text: "This wallpaper has no customizable properties."
                            color: Theme.dim
                            font.family: Theme.font
                            font.pixelSize: 12
                        }
                        Item { Layout.preferredHeight: 16 }
                    }
                }

                // ---- Engine tab: this monitor's LWE options, GPU, capture --
                ScrollView {
                    id: engineScroll
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    visible: view.showGlobal
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                    ColumnLayout {
                        width: engineScroll.availableWidth
                        spacing: 6
                        Section { text: "LINUX-WALLPAPERENGINE · " + view.targetMonitor.toUpperCase() }
                        Repeater {
                            model: GlobalOptions
                            delegate: DelegateChooser {
                                role: "kind"
                                DelegateChoice { roleValue: 10; VolumeControl {} }
                                DelegateChoice { roleValue: 11; FpsControl {} }
                                DelegateChoice { roleValue: 12; ScalingControl {} }
                                DelegateChoice { roleValue: 13; ClampControl {} }
                                DelegateChoice { roleValue: 14; FlagControl {} }
                            }
                        }

                        Section { text: "GPU" }
                        Row2 {
                            caption: "Render on"
                            Segmented {
                                options: Controller.nvidiaAvailable ? ["DEFAULT", "DRI_PRIME", "NVIDIA"] : ["DEFAULT", "DRI_PRIME"]
                                value: Controller.gpuMode
                                onPicked: v => Controller.gpuMode = v
                            }
                            Stepper {
                                visible: Controller.gpuMode === "DRI_PRIME"
                                implicitWidth: 96
                                from: 0; to: 9
                                value: Controller.driPrimeNumber
                                onModified: v => Controller.driPrimeNumber = v
                            }
                            Item { Layout.fillWidth: true }
                        }
                        Row2 {
                            caption: "GL threaded optimisations off"
                            Toggle { checked: Controller.glThreaded; onToggled: c => Controller.glThreaded = c }
                            Text { text: "fixes GLFW / OpenGL issues"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            Item { Layout.fillWidth: true }
                        }

                        Section { text: "CAPTURE & THEME" }
                        Row2 {
                            caption: "Screenshot on apply"
                            Toggle { checked: Controller.screenshotEnabled; onToggled: c => Controller.screenshotEnabled = c }
                            FlatField {
                                Layout.preferredWidth: 320
                                enabled: Controller.screenshotEnabled
                                opacity: enabled ? 1 : 0.45
                                text: Controller.screenshotPath
                                placeholder: "folder for <monitor>_nyaa_sc.png"
                                onEdited: t => Controller.screenshotPath = t
                            }
                            Stepper {
                                implicitWidth: 104
                                enabled: Controller.screenshotEnabled
                                from: 0; to: 100; suffix: " fr"
                                value: Controller.screenshotDelay
                                onModified: v => Controller.screenshotDelay = v
                            }
                            Item { Layout.fillWidth: true }
                        }
                        Row2 {
                            caption: "Theme from this wallpaper"
                            Toggle { checked: Controller.themeFromWallpaper; onToggled: c => Controller.themeFromWallpaper = c }
                            Segmented {
                                options: Controller.pywalAvailable ? ["built-in", "pywal"] : ["built-in"]
                                upper: false
                                value: Controller.themeVia === "pywal" ? "pywal" : "built-in"
                                opacity: Controller.themeFromWallpaper ? 1 : 0.45
                                onPicked: v => Controller.themeVia = (v === "pywal" ? "pywal" : "builtin")
                            }
                            Text {
                                text: Controller.pywalAvailable ? "applied with the wallpaper" : "pywal not installed"
                                color: Theme.dim; font.family: Theme.font; font.pixelSize: 11
                            }
                            Item { Layout.fillWidth: true }
                        }
                        Item { Layout.preferredHeight: 16 }
                    }
                }

                // ---- bottom bar ------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    color: Theme.panel
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 14
                        spacing: 10
                        Text {
                            Layout.fillWidth: true
                            text: view.status
                            color: view.status.startsWith("Error") ? Theme.danger
                                   : view.status.startsWith("Applied") ? Theme.ok : Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 12
                            elide: Text.ElideRight
                        }
                        ActionButton { text: "Stop on " + view.targetMonitor; keyHint: "S"; outline: true; color: Theme.danger; onClicked: view.stopCurrent() }
                        ActionButton { text: "Apply to " + view.targetMonitor; keyHint: "⌃↵"; color: Theme.accent; bold: true; onClicked: view.applyCurrent() }
                    }
                }
            }
        }
    }

    // ---- mini panel: right side, hover strip, 200ms slide, 400ms hide -----
    readonly property int panelWidth: 300

    Timer {
        id: hideTimer
        interval: 400
        onTriggered: view.miniOpen = false
    }

    MouseArea {
        width: 10
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        hoverEnabled: true
        onEntered: { hideTimer.stop(); view.miniOpen = true }
    }

    Rectangle {
        id: miniPanel
        width: view.panelWidth
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        transform: Translate {
            x: view.miniOpen ? 0 : view.panelWidth
            Behavior on x { NumberAnimation { duration: 200 } }
        }
        color: Theme.panel
        border.color: Theme.line
        border.width: 1
        z: 20

        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            onEntered: hideTimer.stop()
            onExited: hideTimer.start()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                // MiniPlaylistPanel: "Playlist — <monitor>", then the runner's status
                Text {
                    text: "Playlist — " + (Playlist.message !== "" && Playlist.monitor === view.targetMonitor
                                           ? Playlist.message : view.targetMonitor)
                    color: Theme.text
                    font.family: Theme.font
                    font.pixelSize: 13
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "✕"
                    color: "#aaaaaa"
                    font.pixelSize: 14
                    MouseArea { anchors.fill: parent; onClicked: view.miniOpen = false }
                }
            }

            Text {
                visible: Playlist.entries.length === 0
                text: "No entries. Use 'Add to Playlist' button."
                color: Theme.dim
                font.family: Theme.font
                font.pixelSize: 11
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ListView {
                id: miniList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: Playlist.entries
                spacing: 5

                // MiniPlaylistPanel.createEntryRow: 60×34 thumbnail, the
                // wallpaper id, a ✕ to remove, click to select
                delegate: Rectangle {
                    required property int index
                    required property var modelData
                    width: miniList.width
                    height: 42
                    radius: Theme.radius
                    color: rowMouse.containsMouse ? Theme.raise : Theme.card
                    border.width: view.miniSelected === index ? 2 : 1
                    border.color: view.miniSelected === index ? Theme.accent : Theme.line

                    MouseArea {
                        id: rowMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: view.miniSelected = index
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        spacing: 8
                        Image {
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 34
                            source: Library.previewFor(modelData.wallpaperId)
                            fillMode: Image.PreserveAspectFit
                            asynchronous: true
                            sourceSize.width: 120
                            sourceSize.height: 68
                            opacity: status === Image.Ready ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 200 } }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelData.wallpaperId
                            color: Theme.muted
                            font.family: Theme.font
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }
                        Text {
                            text: "✕"
                            color: "#d9534f"
                            font.pixelSize: 10
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -6
                                onClicked: view.miniRemove(index)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 5
                ActionButton { text: "▶"; implicitWidth: 28; onClicked: Playlist.play() }
                ActionButton { text: "⏸"; implicitWidth: 28; onClicked: Playlist.togglePause() }
                ActionButton { text: "⏹"; implicitWidth: 28; onClicked: Playlist.stop() }
                ActionButton { text: "↺"; implicitWidth: 28; onClicked: Playlist.reset() }
                ActionButton { text: "⏮"; implicitWidth: 28; onClicked: Playlist.prev() }
                ActionButton { text: "⏭"; implicitWidth: 28; onClicked: Playlist.next() }
            }
        }
    }

}
