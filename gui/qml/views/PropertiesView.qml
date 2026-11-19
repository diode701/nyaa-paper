import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../components"
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

Item {
    id: view

    signal back

    property string wallpaperId: ""
    property string wallpaperTitle: ""
    property string wallpaperPreview: ""
    property string wallpaperBasePath: ""
    property string targetMonitor: Engine.monitors.length > 0 ? Engine.monitors[0] : ""
    property bool showGlobal: false
    property bool startGlobal: false
    onStartGlobalChanged: if (startGlobal) showGlobal = true

    readonly property int leftWidth: Math.min(540, Math.max(320, view.width * 0.26))

    function handleKey(event) {
        if (event.key === Qt.Key_Escape) {
            back()
            event.accepted = true
        }
    }

    onVisibleChanged: {
        if (visible) {
            Properties.loadWallpaper(wallpaperBasePath)
            list.forceActiveFocus()
        }
    }
    onWallpaperBasePathChanged: if (visible) Properties.loadWallpaper(wallpaperBasePath)

    RowLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 22

        // ---- left: what is selected -------------------------------------
        ColumnLayout {
            Layout.preferredWidth: view.leftWidth
            Layout.fillHeight: true
            spacing: 14

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.round(view.leftWidth * 0.56)
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
                font.pixelSize: Math.max(18, Math.round(view.height * 0.026))
                font.bold: true
                font.letterSpacing: 1
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: view.wallpaperId !== "" ? "id " + view.wallpaperId : ""
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

            Flow {
                Layout.fillWidth: true
                spacing: 8
                Repeater {
                    model: Engine.monitors
                    delegate: Rectangle {
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

        // ---- right: properties ------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                spacing: 18
                Text {
                    text: "CUSTOM"
                    color: view.showGlobal ? Theme.dim : Theme.accent
                    font.pixelSize: 11
                    font.letterSpacing: 3
                    font.bold: true
                    MouseArea { anchors.fill: parent; onClicked: view.showGlobal = false }
                }
                Text {
                    text: "GLOBAL"
                    color: view.showGlobal ? Theme.accent : Theme.dim
                    font.pixelSize: 11
                    font.letterSpacing: 3
                    font.bold: true
                    MouseArea { anchors.fill: parent; onClicked: view.showGlobal = true }
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: view.showGlobal ? "engine defaults (edit in settings, S)"
                                          : Properties.count + " properties"
                    color: Theme.dim
                    font.pixelSize: 11
                }
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
                visible: !view.showGlobal
                model: Properties
                spacing: 2

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

                    width: list.width
                    height: isGroup ? 42 : 50
                    radius: 6
                    color: isGroup ? Theme.panelAlt : "transparent"

                    Text {
                        visible: isGroup
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        text: label
                        color: Theme.accent
                        font.pixelSize: 11
                        font.letterSpacing: 2
                        font.bold: true
                    }

                    Text {
                        visible: !isGroup
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        width: 250
                        text: label
                        color: Theme.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    CheckBox {
                        visible: !isGroup && kind === 0
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        checked: value === true
                        onToggled: Properties.setValue(name, checked)
                    }

                    Row {
                        visible: !isGroup && kind === 1
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        spacing: 10

                        Slider {
                            width: 220
                            from: minimum
                            to: maximum
                            stepSize: step
                            value: typeof settingValue === "number" ? settingValue : 0
                            onMoved: Properties.setValue(name, value)
                        }
                        Text {
                            width: 54
                            text: String(display)
                            color: Theme.muted
                            font.pixelSize: 11
                            font.family: Theme.mono
                        }
                    }

                    ComboBox {
                        visible: !isGroup && kind === 3
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        width: 200
                        model: options
                        currentIndex: Math.max(0, options.indexOf(display))
                        onActivated: Properties.setValue(name, currentText)
                    }

                    TextField {
                        visible: !isGroup && kind !== 0 && kind !== 1 && kind !== 3
                                 && kind !== 4 && kind !== 5
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        width: 240
                        text: String(display)
                        color: Theme.text
                        font.pixelSize: 11
                        selectByMouse: true
                        onEditingFinished: Properties.setValue(name, text)
                        background: Rectangle {
                            color: Theme.panelAlt
                            radius: 5
                            border.color: Theme.line
                        }
                    }
                }

                Keys.onPressed: (event) => {
                    if (event.key === Qt.Key_Escape) {
                        view.back()
                        event.accepted = true
                    }
                }
            }

            ListView {
                id: globalList
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: view.showGlobal
                clip: true
                model: GlobalOptions
                spacing: 2

                delegate: Rectangle {
                    required property int index
                    required property string name
                    required property string label
                    required property int kind
                    required property var settingValue
                    required property var boolValue
                    required property var options

                    width: globalList.width
                    height: kind === 5 ? 42 : 50
                    radius: 6
                    color: "transparent"

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 14
                        width: 250
                        text: label
                        color: Theme.text
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }

                    Row {
                        visible: kind === 0
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        spacing: 12
                        Slider {
                            width: 220
                            from: 0
                            to: 100
                            value: typeof settingValue === "number" ? settingValue : 70
                            onMoved: GlobalOptions.setValue(name, Math.round(value))
                        }
                        CheckBox {
                            text: "mute"
                            checked: boolValue === true
                            onToggled: GlobalOptions.setValue(name, checked)
                        }
                    }

                    Row {
                        visible: kind === 1
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        spacing: 12
                        CheckBox {
                            text: "limit"
                            checked: boolValue === true
                            onToggled: GlobalOptions.setValue(name, checked)
                        }
                        SpinBox {
                            from: 0
                            to: 240
                            value: typeof settingValue === "number" ? settingValue : 30
                            onValueModified: GlobalOptions.setValue(name, value)
                        }
                    }

                    ComboBox {
                        visible: kind >= 2 && kind <= 4
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        width: 220
                        model: options
                        currentIndex: Math.max(0, options.indexOf(String(settingValue)))
                        onActivated: GlobalOptions.setValue(name, currentText)
                    }

                    CheckBox {
                        visible: kind === 5
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: parent.right
                        anchors.rightMargin: 12
                        checked: boolValue === true
                        onToggled: GlobalOptions.setValue(name, checked)
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 6
                spacing: 12

                ActionButton {
                    primary: true
                    text: "Apply"
                    onClicked: {
                        Engine.applyWallpaper(view.targetMonitor, view.wallpaperId, view.wallpaperBasePath)
                        Config.saveApplied(view.targetMonitor, view.wallpaperId, Properties.values())
                    }
                }
                ActionButton {
                    text: "Load last"
                    onClicked: {
                        const saved = Config.loadApplied(view.targetMonitor)
                        if (saved.__wallpaperId === view.wallpaperId)
                            Properties.applyValues(saved)
                    }
                }
                ActionButton {
                    text: "Add to playlist"
                    onClicked: Playlist.add(Engine.playlistEntry(view.targetMonitor, view.wallpaperId, view.wallpaperBasePath))
                }
                ActionButton {
                    text: "Stop"
                    onClicked: Engine.stop(view.targetMonitor)
                }
                ActionButton {
                    text: "Back"
                    onClicked: view.back()
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "CTRL+ENTER apply   ·   A add   ·   S stop   ·   ESC back"
                    color: Theme.dim
                    font.pixelSize: 11
                }
            }
        }
    }
}
