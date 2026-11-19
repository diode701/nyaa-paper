import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

/// The browser's status line. Java: WallpaperBrowser.updateStatusBar, a
/// monospace bar. Monitors with their state, page and grid size, the
/// selected title; the key list lives behind "?".
Rectangle {
    id: bar
    property string pageLabel: "1/1"
    property string selection: ""
    property int cols: 1
    property int rows: 1
    implicitHeight: 32
    color: Theme.panel

    function pad(text) {
        const parts = String(text).split("/")
        return parts.map(p => p.length < 2 ? "0" + p : p).join("/")
    }

    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.line }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 14
        spacing: 14

        Repeater {
            model: Engine.monitors
            delegate: Rectangle {
                required property string modelData
                readonly property bool live: Engine.running[modelData] !== undefined
                implicitWidth: tag.implicitWidth + 22
                implicitHeight: 20
                radius: Theme.radius
                color: "transparent"
                border.color: live ? Qt.tint(Theme.line, Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55)) : Theme.line
                Rectangle { x: 7; anchors.verticalCenter: parent.verticalCenter; width: 5; height: 5; color: live ? Theme.accent : Theme.dim }
                Text {
                    id: tag
                    x: 16
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData
                    color: live ? Theme.text : Theme.muted
                    font.family: Theme.font
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                }
            }
        }

        Text {
            text: bar.pad(bar.pageLabel) + " · " + bar.cols + "×" + bar.rows
            color: Theme.text
            font.family: Theme.font
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }
        Text {
            Layout.fillWidth: true
            text: bar.selection
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 12
            elide: Text.ElideRight
        }
        Row {
            spacing: 6
            KeyCap { text: "?"; anchors.verticalCenter: parent.verticalCenter }
            Text { text: "keys"; color: Theme.muted; font.family: Theme.font; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
        }
    }
}
