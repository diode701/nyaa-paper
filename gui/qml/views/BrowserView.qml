import QtQuick
import QtQuick.Controls
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

/// The Java browser: a fixed 180px grid centred in the window, 15px gaps,
/// 20px padding, and a status bar reading "Page X/Y | Title | ←↑↓→ move | …".
Item {
    id: view

    signal open(string wallpaperId, string title, string preview, string basePath)
    signal playlists
    signal requestFocus

    // usable area excludes the 20px padding and the status bar
    readonly property int cols: Math.max(1, Math.floor((width - 2 * Look.pad + Look.gap) / (Look.thumb + Look.gap)))
    readonly property int rows: Math.max(1, Math.floor((height - 26 - 2 * Look.pad + Look.gap) / (Look.thumb + Look.gap)))
    readonly property int pageSize: cols * rows
    readonly property int pageCount: Math.max(1, Math.ceil(Library.count / pageSize))
    readonly property string cursorTitle: (cursor >= 0 && cursor < pageItems.length) ? pageItems[cursor].title : ""

    property int page: 0
    property int cursor: 0
    property string selectedId: ""
    property var pageItems: []
    property bool searchOpen: false

    function rebuild() {
        if (page < 0)
            page = 0
        var start = page * pageSize
        var items = []
        var n = Math.min(pageSize, Math.max(0, Library.count - start))
        for (var i = 0; i < n; ++i)
            items.push(Library.at(start + i))
        pageItems = items
        // selection is remembered by id, like the Java app
        if (selectedId !== "") {
            var found = -1
            for (var j = 0; j < items.length; ++j)
                if (items[j].id === selectedId) { found = j; break }
            cursor = found >= 0 ? found : 0
        }
        if (cursor >= items.length)
            cursor = Math.max(0, items.length - 1)
        if (items.length > 0)
            selectedId = items[cursor].id
    }

    function goTo(newPage) {
        if (newPage < 0 || newPage >= pageCount)
            return
        page = newPage
        rebuild()
    }

    function move(dx, dy) {
        if (pageItems.length === 0)
            return
        var col = cursor % cols
        var row = Math.floor(cursor / cols)
        if (dy < 0 && row === 0 && page > 0) {
            goTo(page - 1)
            cursor = Math.min((rows - 1) * cols + col, pageItems.length - 1)
        } else if (dy > 0 && row === rows - 1 && page < pageCount - 1) {
            goTo(page + 1)
            cursor = Math.min(col, pageItems.length - 1)
        } else {
            var next = cursor + dy * cols + dx
            if (next >= 0 && next < pageItems.length)
                cursor = next
        }
        if (pageItems.length > 0)
            selectedId = pageItems[cursor].id
    }

    function current() { return pageItems.length ? pageItems[cursor] : null }

    function openCurrent() {
        const item = current()
        if (item)
            open(item.id, item.title, item.preview, item.basePath)
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
        page = Math.max(0, Math.min(page, pageCount - 1))
        rebuild()
    }

    Connections {
        target: Library
        function onFilterChanged() { view.page = 0; view.cursor = 0; view.rebuild() }
        function onCountChanged() { view.rebuild() }
    }

    Item {
        anchors.fill: parent
        anchors.bottomMargin: 26

        Grid {
            anchors.centerIn: parent
            anchors.topMargin: Look.pad
            anchors.bottomMargin: Look.pad
            columns: view.cols
            spacing: view.gap

            Repeater {
                model: view.pageItems
                delegate: Cell {
                    preview: view.pageItems[index] ? view.pageItems[index].preview : ""
                    title: view.pageItems[index] ? view.pageItems[index].title : ""
                    width: Look.thumb
                    height: Look.thumb
                    pageStart: view.page * view.pageSize
                    selected: index === view.cursor
                    onClicked: {
                        view.cursor = index
                        view.selectedId = view.pageItems[index].id
                    }
                }
            }

            Keys.onReturnPressed: view.openCurrent()
        }
    }

    Text {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: -20
        visible: Library.count === 0
        text: "nothing here\nadd wallpapers in Steam"
        color: "#aaaaaa"
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
    }

    StatusBar {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: "Page " + (view.page + 1) + "/" + view.pageCount
              + (view.cursorTitle !== "" ? " | " + view.cursorTitle : "")
              + " | ←↑↓→ move | N/P page | Enter select | M playlists | Q quit"
    }

    Rectangle {
        id: dimmer
        anchors.fill: parent
        color: "#B3000000"
        opacity: view.searchOpen ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 140 } }
    }

    Rectangle {
        width: Math.min(560, view.width - 80)
        height: 40
        radius: 4
        color: "#2d2d2d"
        border.width: 1
        border.color: "#3a3a3a"
        anchors.horizontalCenter: parent.horizontalCenter
        y: view.searchOpen ? 20 : -60
        opacity: view.searchOpen ? 1 : 0
        visible: opacity > 0
        Behavior on y { NumberAnimation { duration: 140 } }

        TextField {
            id: search
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            background: Item {}
            color: "#ffffff"
            font.pixelSize: 13
            placeholderText: "search wallpapers"
            placeholderTextColor: "#aaaaaa"
            onTextChanged: Library.filter = text
            Keys.onEscapePressed: view.closeSearch(true)
            Keys.onReturnPressed: {
                if (view.pageItems.length > 0) {
                    view.closeSearch(false)
                    view.openCurrent()
                }
            }
            Keys.onDownPressed: view.closeSearch(false)
        }
    }
}
