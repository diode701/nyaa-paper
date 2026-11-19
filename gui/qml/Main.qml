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

    Shortcut {
        sequence: "Ctrl+Q"
        onActivated: Qt.quit()
    }

    Item {
        anchors.fill: parent

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
                pageLabel: app.view === 0 ? browser.pageLabel : ""
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
