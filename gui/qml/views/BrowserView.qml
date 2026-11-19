import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

Item {
    id: view

    signal open
    signal playlists

    onVisibleChanged: if (visible) grid.forceActiveFocus()

    Rectangle {
        id: searchWrap
        anchors.top: parent.top
        anchors.topMargin: 18
        anchors.horizontalCenter: parent.horizontalCenter
        width: 460
        height: 38
        radius: 19
        color: Theme.panel
        border.width: 1
        border.color: search.activeFocus ? Theme.accent : Theme.line

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 12
            spacing: 10

            TextField {
                id: search
                Layout.fillWidth: true
                placeholderText: "search wallpapers"
                placeholderTextColor: Theme.dim
                color: Theme.text
                font.pixelSize: 13
                background: Item {}
                onTextChanged: Library.filter = text
                Keys.onEscapePressed: {
                    text = ""
                    grid.forceActiveFocus()
                }
            }

            KeyCap { text: "/" }
        }
    }

    GridView {
        id: grid
        anchors.fill: parent
        anchors.topMargin: 74
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.bottomMargin: 16
        clip: true
        focus: true
        currentIndex: 0
        cellWidth: 240
        cellHeight: 172
        cacheBuffer: 1200
        boundsBehavior: Flickable.StopAtBounds
        model: Library
        Component.onCompleted: currentIndex = 0
        highlightFollowsCurrentItem: false

        highlight: Rectangle {
            x: grid.currentItem ? grid.currentItem.x - 4 : 0
            y: grid.currentItem ? grid.currentItem.y - 4 : 0
            width: grid.cellWidth - 8
            height: grid.cellHeight - 8
            radius: Theme.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.accent
            Behavior on x { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
            Behavior on y { NumberAnimation { duration: 90; easing.type: Easing.OutCubic } }
        }

        delegate: Cell {
            width: grid.cellWidth - 16
            height: grid.cellHeight - 16
            onClicked: {
                grid.currentIndex = index
                grid.forceActiveFocus()
                view.open()
            }
        }

        Keys.onPressed: (event) => {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                view.open()
                event.accepted = true
            } else if (event.key === Qt.Key_Slash) {
                search.forceActiveFocus()
                event.accepted = true
            } else if (event.key === Qt.Key_M) {
                view.playlists()
                event.accepted = true
            }
        }
    }

    Text {
        anchors.centerIn: parent
        visible: Library.count === 0
        text: "nothing here\nadd wallpapers in Steam"
        color: Theme.dim
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        lineHeight: 1.4
    }

}
