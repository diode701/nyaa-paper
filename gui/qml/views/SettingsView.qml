import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"
import "../flat"

/// Settings (S from the browser). Sections on the left, 1–4 switch them.
/// Look changes show at once; Save writes config.json, Revert reloads it.
Item {
    id: view

    signal close
    property bool open: false
    property int section: 0
    visible: open
    z: 50

    onOpenChanged: if (open) forceActiveFocus()
    Keys.onPressed: e => {
        if (e.key >= Qt.Key_1 && e.key <= Qt.Key_4) { section = e.key - Qt.Key_1; e.accepted = true }
        else if (e.key === Qt.Key_Escape) { revert(); e.accepted = true }
    }

    function save() { Config.save(); Config.rescanLibrary(); close() }
    function revert() { Config.reload(); close() }
    function pathOf(url) { return decodeURIComponent(url.toString().replace(/^file:\/\//, "")) }

    // ---- building blocks ------------------------------------------------------
    component Heading: Text {
        Layout.fillWidth: true
        Layout.topMargin: 6
        color: Theme.accent
        font.family: Theme.font
        font.pixelSize: 11
        font.weight: Font.Bold
        font.letterSpacing: 1.6
    }
    component Note: Text {
        Layout.fillWidth: true
        color: Theme.dim
        font.family: Theme.font
        font.pixelSize: 11
        wrapMode: Text.WordWrap
    }
    component Line: RowLayout {
        id: line
        property string label: ""
        default property alias content: slot.data
        Layout.fillWidth: true
        spacing: 14
        Text { text: line.label; color: Theme.muted; font.family: Theme.font; font.pixelSize: 12; Layout.preferredWidth: 170 }
        RowLayout { id: slot; spacing: 10 }
        Item { Layout.fillWidth: true }
    }
    component PathLine: RowLayout {
        id: pathLine
        property string label: ""
        property string value: ""
        property bool folder: true
        property string hint: ""
        signal edited(string text)
        Layout.fillWidth: true
        spacing: 14
        Text { text: pathLine.label; color: Theme.muted; font.family: Theme.font; font.pixelSize: 12; Layout.preferredWidth: 170 }
        FlatField { Layout.fillWidth: true; text: pathLine.value; placeholder: pathLine.hint; onEdited: t => pathLine.edited(t) }
        ActionButton { text: "Browse…"; onClicked: pathLine.folder ? folderPick.open() : filePick.open() }
        FolderDialog { id: folderPick; title: pathLine.label; onAccepted: pathLine.edited(view.pathOf(selectedFolder)) }
        FileDialog { id: filePick; title: pathLine.label; onAccepted: pathLine.edited(view.pathOf(selectedFile)) }
    }

    // ---- the dialog -------------------------------------------------------------
    Rectangle {
        anchors.fill: parent
        color: "#b0000000"
        MouseArea { anchors.fill: parent; onClicked: view.revert() }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(1000, parent.width - 60)
        height: Math.min(680, parent.height - 60)
        radius: Theme.radius
        color: Theme.panel
        border.color: Theme.line
        MouseArea { anchors.fill: parent }   // clicks inside do not close

        RowLayout {
            anchors.fill: parent
            spacing: 0

            // ---- section list ----------------------------------------------------
            Rectangle {
                Layout.preferredWidth: 210
                Layout.fillHeight: true
                color: Theme.bg
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.line }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    anchors.topMargin: 18
                    spacing: 3
                    Text {
                        text: "SETTINGS"
                        color: Theme.text
                        font.family: Theme.font
                        font.pixelSize: 13
                        font.weight: Font.ExtraBold
                        font.letterSpacing: 3
                        Layout.leftMargin: 8
                        Layout.bottomMargin: 12
                    }
                    Repeater {
                        model: ["Appearance", "Paths", "Monitors", "Startup"]
                        delegate: Rectangle {
                            required property int index
                            required property string modelData
                            readonly property bool chosen: view.section === index
                            Layout.fillWidth: true
                            implicitHeight: 36
                            radius: Theme.radius
                            color: chosen ? Theme.card : (navHover.hovered ? Qt.lighter(Theme.bg, 1.3) : "transparent")
                            HoverHandler { id: navHover }
                            Rectangle { width: 2; height: parent.height; color: Theme.accent; visible: parent.chosen }
                            Text {
                                x: 12; anchors.verticalCenter: parent.verticalCenter
                                text: modelData
                                color: parent.chosen ? Theme.text : Theme.muted
                                font.family: Theme.font; font.pixelSize: 12
                            }
                            KeyCap { anchors.right: parent.right; anchors.rightMargin: 8; anchors.verticalCenter: parent.verticalCenter; text: String(index + 1) }
                            MouseArea { anchors.fill: parent; onClicked: view.section = index }
                        }
                    }
                    Item { Layout.fillHeight: true }
                    Text {
                        Layout.fillWidth: true
                        Layout.leftMargin: 8
                        text: Config.filePath()
                        color: Theme.dim
                        font.family: Theme.font
                        font.pixelSize: 9
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }

            // ---- section content -------------------------------------------------
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                ScrollView {
                    id: pane
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                    ColumnLayout {
                        width: pane.availableWidth - 48
                        x: 24
                        spacing: 14

                        Item { Layout.preferredHeight: 8 }

                        // ======== Appearance ========
                        ColumnLayout {
                            visible: view.section === 0
                            Layout.fillWidth: true
                            spacing: 14

                            Heading { text: "THEME SOURCE" }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                columnSpacing: 8
                                rowSpacing: 8
                                Repeater {
                                    model: [
                                        { key: "palette", name: "Palette", note: "one of the palettes below" },
                                        { key: "wallpaper", name: "From wallpaper", note: "colours of a wallpaper, extracted in-app" },
                                        { key: "pywal", name: "pywal", note: "reads " + Config.pywalPath + "/colors.json" },
                                        { key: "system", name: "System", note: "follows the Qt / KDE colour scheme" }]
                                    delegate: Rectangle {
                                        required property var modelData
                                        readonly property bool chosen: Config.themeSource === modelData.key
                                        Layout.fillWidth: true
                                        implicitHeight: srcCol.implicitHeight + 20
                                        radius: Theme.radius
                                        color: Theme.card
                                        border.color: chosen ? Theme.accent : Theme.line
                                        ColumnLayout {
                                            id: srcCol
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 3
                                            Text { text: modelData.name; color: Theme.text; font.family: Theme.font; font.pixelSize: 12; font.weight: Font.Bold }
                                            Text { Layout.fillWidth: true; text: modelData.note; color: Theme.dim; font.family: Theme.font; font.pixelSize: 10; wrapMode: Text.WordWrap }
                                        }
                                        MouseArea { anchors.fill: parent; onClicked: Config.themeSource = modelData.key }
                                    }
                                }
                            }

                            Heading { text: "PALETTE" }
                            GridLayout {
                                Layout.fillWidth: true
                                columns: 3
                                columnSpacing: 10
                                rowSpacing: 10
                                Repeater {
                                    model: Theme.palettes
                                    delegate: Rectangle {
                                        id: palCard
                                        required property var modelData
                                        readonly property bool chosen: Config.palette === modelData.key
                                        Layout.fillWidth: true
                                        implicitHeight: 82
                                        radius: Theme.radius
                                        color: Theme.card
                                        border.color: chosen ? Theme.accent : Theme.line
                                        ColumnLayout {
                                            anchors.fill: parent
                                            anchors.margins: 10
                                            spacing: 6
                                            Row {
                                                id: swatches
                                                Layout.fillWidth: true
                                                Repeater {
                                                    model: palCard.modelData.swatch
                                                    delegate: Rectangle {
                                                        required property var modelData
                                                        width: swatches.width / 7
                                                        height: 22
                                                        color: modelData
                                                    }
                                                }
                                            }
                                            Text { text: palCard.modelData.name; color: Theme.text; font.family: Theme.font; font.pixelSize: 12; font.weight: Font.Bold }
                                            Text { text: palCard.modelData.subtitle; color: Theme.dim; font.family: Theme.font; font.pixelSize: 10; elide: Text.ElideRight; Layout.fillWidth: true }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            onClicked: { Config.palette = palCard.modelData.key; Config.themeSource = "palette" }
                                        }
                                    }
                                }
                            }

                            Heading { text: "LOOK" }
                            Line { label: "Hover"
                                Segmented { options: ["java", "lift", "off"]; value: Config.hoverStyle; onPicked: v => Config.hoverStyle = v }
                                Text { text: "java = the original: 2 % zoom and a light border"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "Selection"
                                Segmented { options: ["brackets", "outline", "quiet"]; value: Config.selectionStyle; onPicked: v => Config.selectionStyle = v }
                            }
                            Line { label: "Wallpaper backdrop"
                                Toggle { checked: Config.backdrop; onToggled: c => Config.backdrop = c }
                                Text { text: "the selected wallpaper, blurred, behind the grid"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "Tint from wallpaper"
                                Toggle { checked: Config.tint; onToggled: c => Config.tint = c }
                                Text { text: "the window takes the selected wallpaper's colours"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }

                            Heading { text: "PERFORMANCE" }
                            Line { label: "GIF previews"
                                Segmented { options: ["always", "hover", "off"]; value: Config.animation; onPicked: v => Config.animation = v }
                                Text { text: "hover suits low-end machines · G cycles"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "Renderer"
                                Segmented { options: ["vulkan", "opengl", "software"]; value: Config.renderer; onPicked: v => Config.renderer = v }
                                Text { text: "applies on restart"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "UI scale"
                                FlatSlider {
                                    Layout.preferredWidth: 260
                                    from: 0.75; to: 2.0; stepSize: 0.05
                                    value: Config.uiScale
                                    onMoved: Config.uiScale = Math.round(value * 20) / 20
                                }
                                Text { text: Math.round(Config.uiScale * 100) + " % · applies on restart"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                        }

                        // ======== Paths ========
                        ColumnLayout {
                            visible: view.section === 1
                            Layout.fillWidth: true
                            spacing: 12
                            Heading { text: "PATHS" }
                            PathLine { label: "Workshop"; value: Config.workshopPath; hint: "…/steamapps/workshop/content/431960"; onEdited: t => Config.workshopPath = t }
                            PathLine { label: "Engine assets"; value: Config.assetsPath; hint: "empty: the engine's built-in assets"; onEdited: t => Config.assetsPath = t }
                            PathLine { label: "Engine binary"; value: Config.enginePath; folder: false; hint: "empty: linux-wallpaperengine on PATH"; onEdited: t => Config.enginePath = t }
                            PathLine { label: "Screenshots"; value: Config.screenshotsPath; onEdited: t => Config.screenshotsPath = t }
                            PathLine { label: "pywal cache"; value: Config.pywalPath; onEdited: t => Config.pywalPath = t }
                            Note { text: "The daemon re-reads these when you save; the library is rescanned." }
                            Line { label: "Wayland layer"
                                // saved but not passed on: the engine has no --layer yet
                                enabled: false
                                opacity: 0.5
                                Segmented {
                                    options: ["none", "background", "bottom", "top", "overlay"]
                                    value: Config.layer === "" ? "none" : Config.layer
                                    onPicked: v => Config.layer = (v === "none" ? "" : v)
                                }
                            }
                            Note { text: "Wayland layer: not implemented yet. The engine draws on the background layer; to change stacking, use your compositor's rules (see README)." }
                        }

                        // ======== Monitors ========
                        ColumnLayout {
                            visible: view.section === 2
                            Layout.fillWidth: true
                            spacing: 12
                            Heading { text: "MONITORS" }
                            MonitorMap {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 160
                                selected: ""
                            }
                            Note { text: "Detected: " + Engine.monitors.join(", ") }
                            Line { label: "Extra monitors"
                                FlatField {
                                    Layout.preferredWidth: 380
                                    text: Config.customMonitors
                                    placeholder: "outputs Qt does not report, e.g. DP-2, HDMI-A-3"
                                    onEdited: t => Config.customMonitors = t
                                }
                            }
                        }

                        // ======== Startup ========
                        ColumnLayout {
                            visible: view.section === 3
                            Layout.fillWidth: true
                            spacing: 12
                            Heading { text: "STARTUP" }
                            Line { label: "Restore wallpapers"
                                Toggle { checked: Config.persistOnStart; onToggled: c => Config.persistOnStart = c }
                                Text { text: "put each monitor's last wallpaper back when the app starts"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "Start on login"
                                Toggle { checked: Config.autoStartOnBoot; onToggled: c => Config.autoStartOnBoot = c }
                                Text { text: "hidden in the tray; also lets the session start the daemon"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                            Line { label: "Title screen"
                                Toggle { checked: Config.titleScreen; onToggled: c => Config.titleScreen = c }
                                Text { text: "start on the title screen; off goes straight to the browser"; color: Theme.dim; font.family: Theme.font; font.pixelSize: 11 }
                            }
                        }
                        Item { Layout.preferredHeight: 12 }
                    }
                }

                // ---- save / revert ------------------------------------------------
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    color: Theme.panel
                    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 24
                        anchors.rightMargin: 14
                        spacing: 8
                        Note { text: "Look changes show now; Save keeps them." }
                        ActionButton { text: "Revert"; keyHint: "ESC"; onClicked: view.revert() }
                        ActionButton { text: "Save"; color: Theme.accent; bold: true; onClicked: view.save() }
                    }
                }
            }
        }
    }
}
