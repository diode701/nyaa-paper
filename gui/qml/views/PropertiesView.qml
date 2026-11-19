import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

/// A 1:1 port of WallpaperPropertiesView: top bar, tab pane with one card per
/// setting, GPU section, Screenshot section, bottom bar with status + buttons.
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

    function toggleMini() {
        miniOpen = !miniOpen
        if (miniOpen)
            Playlist.load(targetMonitor)
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

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- top bar ------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#2d2d2d"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                ActionButton { text: "← Back"; onClicked: view.back() }

                ComboBox {
                    id: monitorCombo
                    Layout.preferredWidth: 140
                    implicitHeight: 30
                    font.pixelSize: 12
                    editable: true
                    model: Engine.monitors
                    currentIndex: Math.max(0, Engine.monitors.indexOf(view.targetMonitor))
                    onActivated: view.targetMonitor = currentText
                    onAccepted: view.targetMonitor = editText
                    background: Rectangle { color: "#444444"; radius: 3; border.color: "#555555" }
                }

                ActionButton { text: "Load last"; onClicked: view.loadLast() }
                ActionButton { text: "Add to Playlist"; color: Theme.success; onClicked: view.addCurrent() }
                ActionButton { text: "▶"; implicitWidth: 32; onClicked: view.toggleMini() }
                Item { Layout.fillWidth: true }

                Text {
                    text: "Properties: " + view.wallpaperTitle
                    color: "#ffffff"
                    font.pixelSize: 16
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.maximumWidth: view.width * 0.4
                }
            }
        }

        // ---- tabs ---------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 34
            color: "#2d2d2d"

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.bottom: parent.bottom
                Repeater {
                    model: ["Custom Properties", "Global Options"]
                    delegate: Rectangle {
                        required property int index
                        required property string modelData
                        width: tabLabel.implicitWidth + 30
                        height: 30
                        color: (index === 1) === view.showGlobal ? "#1e1e1e" : "#3a3a3a"
                        border.color: "#555555"
                        Text {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: modelData
                            color: "#ffffff"
                            font.pixelSize: 12
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: view.showGlobal = index === 1
                        }
                    }
                }
            }
        }

        // ---- custom properties -------------------------------------------
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !view.showGlobal
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 15
                Layout.margins: 20

                Repeater {
                    model: Properties
                    delegate: Rectangle {
                        required property int index
                        required property int kind
                        required property string name
                        required property string label
                        required property var display
                        required property var settingValue
                        required property var minimum
                        required property var maximum
                        required property var step
                        required property var options
                        required property bool isGroup

                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        implicitHeight: inner.implicitHeight + 20
                        radius: 5
                        color: isGroup ? "#333333" : "#2a2a2a"

                        ColumnLayout {
                            id: inner
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 5

                            // bool: checkbox carries its own label
                            CheckBox {
                                visible: kind === 0
                                text: label
                                checked: settingValue === true
                                font.pixelSize: 12
                                onToggled: Properties.setValue(name, checked)
                            }

                            // slider: label + slider + value
                            RowLayout {
                                visible: kind === 1
                                spacing: 10
                                Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
                                Slider {
                                    Layout.preferredWidth: 220
                                    from: minimum
                                    to: maximum
                                    stepSize: step
                                    value: typeof settingValue === "number" ? settingValue : 0
                                    onMoved: Properties.setValue(name, value)
                                }
                                Text {
                                    text: Number(settingValue).toFixed(2)
                                    color: "#d3d3d3"
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 44
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // color: label + swatch/hex field
                            RowLayout {
                                visible: kind === 2
                                spacing: 10
                                Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
                                Rectangle {
                                    width: 24; height: 24; radius: 3
                                    border.color: "#555555"
                                    color: String(display).match(/^#[0-9a-fA-F]{6}$/) ? String(display) : "#000000"
                                }
                                TextField {
                                    Layout.preferredWidth: 140
                                    text: String(display)
                                    color: "#ffffff"
                                    font.pixelSize: 12
                                    onEditingFinished: Properties.setValue(name, text)
                                }
                                Item { Layout.fillWidth: true }
                            }

                            // combo: label above, box below
                            Text { visible: kind === 3; text: label; color: "#d3d3d3"; font.pixelSize: 12 }
                            ComboBox {
                                visible: kind === 3
                                Layout.preferredWidth: 240
                                implicitHeight: 30
                                font.pixelSize: 12
                                model: options
                                currentIndex: Math.max(0, options.indexOf(String(display)))
                                onActivated: Properties.setValue(name, currentText)
                                background: Rectangle { color: "#444444"; radius: 3; border.color: "#555555" }
                            }

                            // text: a wrapped grey label
                            Text {
                                visible: kind === 4
                                text: label
                                color: "#aaaaaa"
                                font.pixelSize: 13
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            // group: bold header
                            Text {
                                visible: isGroup
                                text: label
                                color: "#e0e0e0"
                                font.pixelSize: 14
                                font.bold: true
                                wrapMode: Text.WordWrap
                                Layout.fillWidth: true
                            }

                            // textinput / file / directory / texture
                            Text {
                                visible: kind === 6 || kind === 7 || kind === 8 || kind === 9
                                text: label
                                color: "#d3d3d3"
                                font.pixelSize: 12
                            }
                            RowLayout {
                                visible: kind === 6 || kind === 7 || kind === 8 || kind === 9
                                spacing: 8
                                TextField {
                                    Layout.preferredWidth: 350
                                    text: String(display)
                                    color: "#ffffff"
                                    font.pixelSize: 12
                                    selectByMouse: true
                                    onEditingFinished: Properties.setValue(name, text)
                                }
                                ActionButton {
                                    visible: kind === 7 || kind === 8 || kind === 9
                                    text: "Browse..."
                                    onClicked: {
                                        browse.kind = kind
                                        browse.name = name
                                        if (kind === 8) folderDialog.open()
                                        else fileDialog.open()
                                    }
                                }
                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }

                Text {
                    visible: Properties.count === 0
                    text: "This wallpaper has no customizable properties."
                    color: "#d3d3d3"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                }
            }
        }

        // ---- global options ----------------------------------------------
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: view.showGlobal
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 15

                Repeater {
                    model: GlobalOptions
                    delegate: Rectangle {
                        required property string name
                        required property string label
                        required property int kind
                        required property var settingValue
                        required property var boolValue
                        required property var options

                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        implicitHeight: inner.implicitHeight + 20
                        radius: 5
                        color: "#2a2a2a"

                        ColumnLayout {
                            id: inner
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.margins: 10
                            spacing: 5

                            Text {
                                text: label
                                color: "#e0e0e0"
                                font.bold: true
                                font.pixelSize: 12
                                visible: kind !== 5
                            }

                            RowLayout {
                                visible: kind === 0
                                spacing: 10
                                CheckBox {
                                    text: "Mute"
                                    checked: boolValue === true
                                    onToggled: GlobalOptions.setValue(name, checked)
                                }
                                Slider {
                                    Layout.preferredWidth: 220
                                    from: 0
                                    to: 100
                                    stepSize: 5
                                    value: typeof settingValue === "number" ? settingValue : 70
                                    enabled: !boolValue
                                    onMoved: GlobalOptions.setValue(name, Math.round(value))
                                }
                                Text {
                                    text: String(settingValue)
                                    color: "#d3d3d3"
                                    font.pixelSize: 12
                                    Layout.preferredWidth: 34
                                }
                                Item { Layout.fillWidth: true }
                            }

                            RowLayout {
                                visible: kind === 1
                                spacing: 10
                                CheckBox {
                                    text: "Limit FPS"
                                    checked: boolValue === true
                                    onToggled: GlobalOptions.setValue(name, checked)
                                }
                                SpinBox {
                                    from: 0
                                    to: 240
                                    value: typeof settingValue === "number" ? settingValue : 30
                                    enabled: boolValue === true
                                    onValueModified: GlobalOptions.setValue(name, value)
                                }
                                Item { Layout.fillWidth: true }
                            }

                            ComboBox {
                                visible: kind === 2
                                Layout.preferredWidth: 240
                                implicitHeight: 30
                                font.pixelSize: 12
                                model: ["stretch", "fit", "fill", "default"]
                                currentIndex: Math.max(0, model.indexOf(String(settingValue)))
                                onActivated: GlobalOptions.setValue(name, currentText)
                            }

                            RowLayout {
                                visible: kind === 3
                                spacing: 10
                                CheckBox {
                                    text: "Enable clamping"
                                    checked: boolValue === true
                                    onToggled: GlobalOptions.setValue(name, checked)
                                }
                                ComboBox {
                                    Layout.preferredWidth: 160
                                    implicitHeight: 30
                                    font.pixelSize: 12
                                    model: ["clamp", "border", "repeat"]
                                    currentIndex: Math.max(0, model.indexOf(String(settingValue)))
                                    enabled: boolValue === true
                                    onActivated: GlobalOptions.setValue(name, currentText)
                                }
                                Item { Layout.fillWidth: true }
                            }

                            CheckBox {
                                visible: kind === 5
                                text: label
                                checked: boolValue === true
                                font.pixelSize: 12
                                onToggled: GlobalOptions.setValue(name, checked)
                            }
                        }
                    }
                }
            }
        }

        // ---- GPU ----------------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            implicitHeight: gpuColumn.implicitHeight + 20
            radius: 5
            color: "#2a2a2a"

            ColumnLayout {
                id: gpuColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                Text { text: "GPU"; color: "#ffffff"; font.pixelSize: 16 }

                RadioButton {
                    text: "Default (auto)"
                    checked: Controller.gpuMode === "DEFAULT"
                    onToggled: if (checked) Controller.gpuMode = "DEFAULT"
                }
                RadioButton {
                    text: "DRI_PRIME (AMD/Intel)"
                    checked: Controller.gpuMode === "DRI_PRIME"
                    onToggled: if (checked) Controller.gpuMode = "DRI_PRIME"
                }
                RowLayout {
                    spacing: 10
                    Text { text: "GPU index:"; color: "#d3d3d3"; font.pixelSize: 12 }
                    SpinBox {
                        from: 0
                        to: 9
                        value: Controller.driPrimeNumber
                        enabled: Controller.gpuMode === "DRI_PRIME"
                        onValueModified: Controller.driPrimeNumber = value
                    }
                    Item { Layout.fillWidth: true }
                }
                RadioButton {
                    text: "NVIDIA Prime Offload"
                    checked: Controller.gpuMode === "NVIDIA"
                    onToggled: if (checked) Controller.gpuMode = "NVIDIA"
                }
                CheckBox {
                    text: "Disable GL threaded optimizations (fixes GLFW/OpenGL issues)"
                    checked: Controller.glThreaded
                    font.pixelSize: 12
                    onToggled: Controller.glThreaded = checked
                }
            }
        }

        // ---- screenshot ---------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 20
            Layout.rightMargin: 20
            implicitHeight: shotColumn.implicitHeight + 20
            radius: 5
            color: "#2a2a2a"

            ColumnLayout {
                id: shotColumn
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                spacing: 6

                Text { text: "📸 Screenshot (for PyWAL etc.)"; color: "#ffffff"; font.pixelSize: 16 }
                CheckBox {
                    text: "Generate screenshot on apply"
                    checked: Controller.screenshotEnabled
                    onToggled: Controller.screenshotEnabled = checked
                }
                RowLayout {
                    spacing: 10
                    Text { text: "Output file:"; color: "#d3d3d3"; font.pixelSize: 12 }
                    TextField {
                        Layout.preferredWidth: 300
                        text: Controller.screenshotPath
                        enabled: Controller.screenshotEnabled
                        placeholderText: "/path/to/screenshot.png"
                        onEditingFinished: Controller.screenshotPath = text
                    }
                    Item { Layout.fillWidth: true }
                }
                RowLayout {
                    spacing: 10
                    Text { text: "Delay (frames):"; color: "#d3d3d3"; font.pixelSize: 12 }
                    SpinBox {
                        from: 0
                        to: 100
                        value: Controller.screenshotDelay
                        enabled: Controller.screenshotEnabled
                        onValueModified: Controller.screenshotDelay = value
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }

        // ---- bottom bar ---------------------------------------------------
        Rectangle {
            Layout.fillWidth: true
            height: 62
            color: "#2d2d2d"

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 15
                anchors.rightMargin: 15
                spacing: 10

                Text {
                    text: view.status
                    color: "#cccccc"
                    font.pixelSize: 12
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                ActionButton {
                    text: "Apply to Monitor"
                    color: Theme.accent
                    bold: true
                    onClicked: view.applyCurrent()
                }
                ActionButton {
                    text: "Stop on Monitor"
                    color: Theme.danger
                    onClicked: view.stopCurrent()
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
        color: "#252525"
        border.color: "#3a3a3a"
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
                Text { text: "Playlist — " + view.targetMonitor; color: "#ffffff"; font.pixelSize: 14 }
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
                color: "#aaaaaa"
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

                delegate: Rectangle {
                    required property int index
                    required property var modelData
                    width: miniList.width
                    height: 34
                    radius: 4
                    color: Playlist.index === index ? "#333333" : "#2a2a2a"
                    border.color: Playlist.index === index ? Theme.accent : "#3a3a3a"
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        text: modelData.wallpaperId
                        color: "#d3d3d3"
                        font.pixelSize: 11
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

    FileDialog {
        id: fileDialog
        title: "Select file"
        onAccepted: Properties.setValue(browse.name, selectedFile.toString().replace("file://", ""))
    }
    FolderDialog {
        id: folderDialog
        title: "Select directory"
        onAccepted: Properties.setValue(browse.name, selectedFolder.toString().replace("file://", ""))
    }
    QtObject {
        id: browse
        property int kind: 7
        property string name: ""
    }
}
