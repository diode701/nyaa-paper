import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: view

    signal close
    property bool open: false
    visible: open
    z: 50

    component PathRow: RowLayout {
        id: row
        property string label: ""
        property string value: ""
        signal edited(string text)
        Layout.fillWidth: true
        spacing: 12
        Text {
            text: row.label
            color: Theme.muted
            font.pixelSize: 11
            Layout.preferredWidth: 130
        }
        TextField {
            Layout.fillWidth: true
            text: row.value
            color: Theme.text
            font.pixelSize: 12
            selectByMouse: true
            onTextEdited: row.edited(text)
            background: Rectangle {
                color: Theme.panelAlt
                radius: 6
                border.color: Theme.line
            }
        }
    }

    component OptionRow: RowLayout {
        id: row
        property string label: ""
        property var options: []
        property string value: ""
        signal picked(string value)
        Layout.fillWidth: true
        spacing: 12
        Text {
            text: row.label
            color: Theme.muted
            font.pixelSize: 11
            Layout.preferredWidth: 130
        }
        ComboBox {
            Layout.preferredWidth: 220
            model: row.options
            currentIndex: Math.max(0, row.options.indexOf(row.value))
            onActivated: row.picked(currentText)
        }
        Item { Layout.fillWidth: true }
    }

    component Section: Text {
        Layout.fillWidth: true
        Layout.topMargin: 8
        color: Theme.accent
        font.pixelSize: 10
        font.letterSpacing: 3
        font.bold: true
    }

    Rectangle {
        anchors.fill: parent
        color: "#cc000000"
        MouseArea {
            anchors.fill: parent
            onClicked: view.close()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(780, parent.width - 80)
        height: Math.min(640, parent.height - 80)
        radius: 10
        color: Theme.panel
        border.color: Theme.line

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "SETTINGS"
                    color: Theme.text
                    font.pixelSize: 13
                    font.letterSpacing: 4
                    font.bold: true
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: Config.filePath()
                    color: Theme.dim
                    font.pixelSize: 10
                    font.family: Theme.mono
                }
            }

            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.line }

            ScrollView {
                id: scroll0
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ColumnLayout {
                    width: scroll0.availableWidth
                    spacing: 10

                    Section { text: "PATHS" }
                    PathRow { label: "Workshop"; value: Config.workshopPath; onEdited: Config.workshopPath = text }
                    PathRow { label: "Engine binary"; value: Config.enginePath; onEdited: Config.enginePath = text }
                    PathRow { label: "Engine assets"; value: Config.assetsPath; onEdited: Config.assetsPath = text }
                    PathRow { label: "Screenshots"; value: Config.screenshotsPath; onEdited: Config.screenshotsPath = text }
                    PathRow { label: "PyWAL cache"; value: Config.pywalPath; onEdited: Config.pywalPath = text }

                    Section { text: "MONITORS" }
                    PathRow {
                        label: "Extra monitors"
                        value: Config.customMonitors
                        onEdited: Config.customMonitors = text
                    }
                    Text {
                        text: "Detected: " + Engine.monitors.join(", ") + ". Add outputs Qt does not report, comma separated (e.g. DP-2)."
                        color: Theme.dim
                        font.pixelSize: 10
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }

                    Section { text: "STARTUP" }
                    CheckBox {
                        text: "Restore wallpapers on start"
                        checked: Config.persistOnStart
                        onToggled: Config.persistOnStart = checked
                    }
                    CheckBox {
                        text: "Start on login (runs in tray)"
                        checked: Config.autoStartOnBoot
                        onToggled: Config.autoStartOnBoot = checked
                    }

                    Section { text: "PREVIEWS" }
                    OptionRow {
                        label: "Animated previews"
                        options: ["always", "hover", "off"]
                        value: Config.animation
                        onPicked: Config.animation = value
                    }

                    Section { text: "ENGINE" }
                    // Volume, FPS, scaling, clamp and flags are per wallpaper and
                    // per monitor (Properties → Global Options), as in Java.
                    OptionRow {
                        label: "Wayland layer"
                        options: ["", "background", "bottom", "top", "overlay"]
                        value: Config.layer
                        onPicked: Config.layer = value
                    }

                    Item { Layout.preferredHeight: 8 }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "Changes apply on save. The daemon re-reads the file on rescan."
                    color: Theme.dim
                    font.pixelSize: 10
                }
                Item { Layout.fillWidth: true }
                ActionButton {
                    text: "Revert"
                    onClicked: {
                        Config.reload()
                        view.close()
                    }
                }
                ActionButton {
                    text: "Save"
                    onClicked: {
                        Config.save()
                        Config.rescanLibrary()
                        view.close()
                    }
                }
            }
        }
    }
}
