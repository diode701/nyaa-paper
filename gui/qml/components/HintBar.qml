import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Rectangle {
    id: root
    property int mode: 0
    property string pageLabel: ""
    property string selection: ""
    implicitHeight: 34
    color: Theme.panel

    readonly property int runningCount: Object.keys(Engine.running).length

    function hints() {
        if (mode === 0)
            return [["\u2190\u2191\u2193\u2192", "move"], ["ENTER", "open"], ["/", "search"],
                    ["M", "playlists"], ["G", "animations"], ["CTRL+Q", "quit"]]
        if (mode === 1)
            return [["CTRL+ENTER", "apply"], ["S", "stop"], ["L", "load last"],
                    ["A", "add to playlist"], ["ESC", "back"]]
        return [["\u2191\u2193", "select"], ["SPACE", "play / pause"], ["DEL", "remove"], ["ESC", "back"]]
    }

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.line
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 18
        anchors.rightMargin: 18
        spacing: 16

        Repeater {
            model: root.hints()
            delegate: RowLayout {
                required property var modelData
                spacing: 5
                KeyCap { text: modelData[0] }
                Text { text: modelData[1]; color: Theme.muted; font.pixelSize: 10 }
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            text: Settings.animationMode === 2 ? "gif always"
                : Settings.animationMode === 1 ? "gif on hover" : "gif off"
            color: Theme.dim
            font.pixelSize: 10
        }
        Text {
            visible: Engine.mock
            text: "mock"
            color: Theme.accent
            font.pixelSize: 10
            font.family: Theme.mono
        }
        RowLayout {
            spacing: 5
            visible: root.runningCount > 0
            Rectangle {
                width: 5
                height: 5
                radius: 3
                color: Theme.accent
            }
            Text {
                text: root.runningCount + " running"
                color: Theme.muted
                font.pixelSize: 10
            }
        }
        Text {
            visible: root.selection !== ""
            text: root.selection
            color: Theme.text
            font.pixelSize: 10
            elide: Text.ElideRight
            Layout.maximumWidth: 320
        }
        Text {
            text: Library.count + " wallpapers" + (root.pageLabel !== "" ? "  " + root.pageLabel : "")
            color: Theme.dim
            font.pixelSize: 10
            font.family: Theme.mono
        }
    }
}
