import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

/// "?" in the browser: every key on one card, instead of a crowded status bar.
Rectangle {
    id: card
    property bool open: false
    width: 330
    height: grid.implicitHeight + 26
    radius: Theme.radius
    color: Theme.panel
    border.color: Theme.line
    visible: opacity > 0
    opacity: open ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: 120 } }

    readonly property var keys: [
        ["←↑↓→", "move · the edges turn the page"],
        ["N  P", "previous / next page"],
        ["ENTER", "open properties"],
        ["/", "search"],
        ["M", "playlists"],
        ["S", "settings"],
        ["G", "gif previews: " + (Settings.animationMode === 2 ? "always" : Settings.animationMode === 1 ? "hover" : "off")],
        ["CTRL+Q", "quit (wallpapers keep running)"],
    ]

    GridLayout {
        id: grid
        anchors.fill: parent
        anchors.margins: 13
        columns: 2
        columnSpacing: 12
        rowSpacing: 7
        Text {
            Layout.columnSpan: 2
            text: "BROWSER"
            color: Theme.accent
            font.family: Theme.font
            font.pixelSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 1.6
        }
        Repeater {
            model: card.keys.length * 2
            delegate: Loader {
                required property int index
                readonly property var row: card.keys[Math.floor(index / 2)]
                sourceComponent: index % 2 === 0 ? capComponent : textComponent
                Component { id: capComponent; KeyCap { text: row[0] } }
                Component {
                    id: textComponent
                    Text { text: row[1]; color: Theme.muted; font.family: Theme.font; font.pixelSize: 11 }
                }
            }
        }
    }
}
