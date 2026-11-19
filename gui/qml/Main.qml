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
    minimumWidth: 900
    minimumHeight: 600
    visible: true
    title: "Nyaa Paper"
    color: Theme.bg

    property int view: 0

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: Theme.panel

                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: 1
                    color: Theme.line
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22
                    anchors.rightMargin: 22
                    spacing: 18

                    Text {
                        text: "NYAA PAPER"
                        color: Theme.text
                        font.pixelSize: 14
                        font.bold: true
                        font.letterSpacing: 6
                    }
                    Rectangle { width: 1; height: 18; color: Theme.line }
                    Text {
                        text: ["LIBRARY", "PROPERTIES", "PLAYLIST"][app.view]
                        color: Theme.muted
                        font.pixelSize: 11
                        font.letterSpacing: 3
                    }
                    Item { Layout.fillWidth: true }

                    Repeater {
                        model: Engine.monitors
                        delegate: MonitorPill {
                            monitor: modelData
                            running: Engine.running[modelData] !== undefined
                        }
                    }

                    Rectangle {
                        visible: Engine.mock
                        implicitWidth: mockLabel.implicitWidth + 16
                        implicitHeight: 22
                        radius: 11
                        color: "transparent"
                        border.color: Theme.accentDim
                        Text {
                            id: mockLabel
                            anchors.centerIn: parent
                            text: "MOCK"
                            color: Theme.accent
                            font.pixelSize: 10
                            font.letterSpacing: 2
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                BrowserView {
                    anchors.fill: parent
                    visible: app.view === 0
                    onOpen: app.view = 1
                    onPlaylists: app.view = 2
                }
                PropertiesView {
                    anchors.fill: parent
                    visible: app.view === 1
                    onBack: app.view = 0
                }
                PlaylistView {
                    anchors.fill: parent
                    visible: app.view === 2
                    onBack: app.view = 0
                }
            }

            HintBar {
                Layout.fillWidth: true
                mode: app.view
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

    Component.onCompleted: Engine.refreshRunning()
}
