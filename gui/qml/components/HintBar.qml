import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Rectangle {
    id: root
    property int mode: 0
    implicitHeight: 40
    color: Theme.panel

    function hints() {
        if (mode === 0)
            return [["\u2191\u2193\u2190\u2192", "move"], ["ENTER", "open"], ["/", "search"],
                    ["M", "playlists"], ["CTRL+Q", "quit"]]
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
        anchors.leftMargin: 22
        anchors.rightMargin: 22
        spacing: 20

        Repeater {
            model: root.hints()
            delegate: RowLayout {
                required property var modelData
                spacing: 6
                KeyCap { text: modelData[0] }
                Text {
                    text: modelData[1]
                    color: Theme.muted
                    font.pixelSize: 11
                }
            }
        }

        Item { Layout.fillWidth: true }

        Text {
            text: Library.count + " wallpapers"
            color: Theme.dim
            font.pixelSize: 11
        }
        Text {
            text: Engine.version
            color: Theme.dim
            font.pixelSize: 11
            font.family: Theme.mono
        }
    }
}
