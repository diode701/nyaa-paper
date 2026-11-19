import QtQuick
import QtQuick.Controls
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

Item {
    id: view

    signal open
    signal playlists
    signal requestFocus

    property string pageLabel: "1/1"

    // The original app's geometry, unchanged: fixed 180px thumbs, 15px gaps,
    // 20px margins, and the status bar taken out of the usable height.
    readonly property int gap: Look.gap
    readonly property int thumb: Look.thumb
    readonly property int pad: Look.pad
    readonly property int cols: Math.max(1, Math.floor((width - 2 * pad + gap) / (thumb + gap)))
    readonly property int rows: Math.max(1, Math.floor((height - 2 * pad + gap) / (thumb + gap)))
    readonly property int pageSize: cols * rows
    readonly property int cellSize: thumb
    readonly property string cursorTitle: (cursor >= 0 && cursor < pageItems.length)
                                          ? pageItems[cursor].title : ""

    readonly property int pageCount: Math.max(1, Math.ceil(Library.count / pageSize))
    property int page: 0
    property int cursor: 0
    property var pageItems: []
    property int flipDir: 1
    property bool searchOpen: false

    function rebuild() {
        var start = page * pageSize
        var items = []
        var n = Math.min(pageSize, Math.max(0, Library.count - start))
        for (var i = 0; i < n; ++i)
            items.push(Library.at(start + i))
        pageItems = items
        pageLabel = (page + 1) + "/" + pageCount
        if (cursor >= items.length)
            cursor = Math.max(0, items.length - 1)
    }

    function goTo(newPage, newCursor) {
        if (newPage < 0 || newPage >= pageCount)
            return
        flipDir = newPage > page ? 1 : -1
        page = newPage
        cursor = Math.max(0, newCursor)
        deck.x = flipDir * 42
        deck.opacity = 0.3
        rebuild()
        deckAnim.restart()
    }

    function move(dx, dy) {
        if (pageItems.length === 0)
            return
        var col = cursor % cols
        var row = Math.floor(cursor / cols)
        if (dy < 0 && row === 0) {
            goTo(page - 1, Math.min((rows - 1) * cols + col, pageSize - 1))
            return
        }
        if (dy > 0 && row === rows - 1) {
            goTo(page + 1, col)
            return
        }
        var next = cursor + dy * cols + dx
        if (next >= 0 && next < pageItems.length)
            cursor = next
    }

    function openSearch() {
        searchOpen = true
        search.forceActiveFocus()
    }

    function closeSearch(clear) {
        searchOpen = false
        if (clear) {
            search.text = ""
            Library.filter = ""
        }
        requestFocus()
    }

    Component.onCompleted: rebuild()
    onVisibleChanged: if (visible) rebuild()
    onPageSizeChanged: {
        page = 0
        cursor = 0
        rebuild()
    }

    Connections {
        target: Library
        function onFilterChanged() {
            view.page = 0
            view.cursor = 0
            view.rebuild()
        }
        function onCountChanged() { view.rebuild() }
    }

    Item {
        id: deck
        anchors.fill: parent

        Grid {
            anchors.centerIn: parent
            columns: view.cols
            spacing: view.gap

            Repeater {
                model: view.pageItems
                delegate: Cell {
                    width: view.thumb
                    height: view.thumb
                    pageStart: view.page * view.pageSize
                    selected: index === view.cursor
                    onClicked: {
                        view.cursor = index
                        view.open()
                    }
                }
            }
        }

        ParallelAnimation {
            id: deckAnim
            NumberAnimation {
                target: deck
                property: "x"
                to: 0
                duration: 170
                easing.type: Easing.OutCubic
            }
            NumberAnimation { target: deck; property: "opacity"; to: 1; duration: 170 }
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

    Rectangle {
        id: dimmer
        anchors.fill: parent
        color: "#B3000000"
        opacity: view.searchOpen ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 140 } }
        MouseArea {
            anchors.fill: parent
            onClicked: view.closeSearch(true)
        }
    }

    Rectangle {
        id: searchPanel
        width: Math.min(560, view.width - 80)
        height: 44
        radius: 22
        color: Theme.panel
        border.width: 1
        border.color: Theme.accent
        anchors.horizontalCenter: parent.horizontalCenter
        y: view.searchOpen ? 26 : -60
        opacity: view.searchOpen ? 1 : 0
        visible: opacity > 0
        Behavior on y { NumberAnimation { duration: 170; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        TextField {
            id: search
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 14
            background: Item {}
            color: Theme.text
            font.pixelSize: 14
            placeholderText: "search wallpapers"
            placeholderTextColor: Theme.dim
            onTextChanged: Library.filter = text
            Keys.onEscapePressed: view.closeSearch(true)
            Keys.onReturnPressed: {
                if (view.pageItems.length > 0) {
                    view.closeSearch(false)
                    view.open()
                }
            }
            Keys.onDownPressed: view.closeSearch(false)
        }
    }
}
