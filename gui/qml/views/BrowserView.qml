import QtQuick
import QtQuick.Controls
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0
import "../components"

Item {
    id: view

    signal open(string wallpaperId, string title, string preview, string basePath)
    signal playlists
    signal requestFocus

    property string pageLabel: "1/1"

    // WallpaperBrowser.recomputeGrid: 180 px thumbs, 15 px gaps, 20 px margins,
    // usable area = window minus 40 x 80. Applied 150 ms after the last resize
    // (Java's resizeDebouncer), keeping the page and the selected wallpaper.
    readonly property int gap: Look.gap
    readonly property int thumb: Look.thumb
    readonly property int liveCols: (width - 40 < thumb || height - 80 < thumb)
                                    ? 1 : Math.max(1, Math.floor((width - 40 + gap) / (thumb + gap)))
    readonly property int liveRows: (width - 40 < thumb || height - 80 < thumb)
                                    ? 1 : Math.max(1, Math.floor((height - 80 + gap) / (thumb + gap)))
    property int cols: 1
    property int rows: 1
    readonly property int pageSize: cols * rows
    readonly property int cellSize: thumb
    readonly property string cursorTitle: (cursor >= 0 && cursor < pageItems.length)
                                          ? pageItems[cursor].title : ""

    readonly property int pageCount: Math.max(1, Math.ceil(Library.count / pageSize))
    property int page: 0
    property int cursor: 0
    property string selectedId: ""      // Java: selectedId, survives reloads
    property var pageItems: []
    property var prefetch: []           // previews of the pages around this one
    property int flipDir: 1
    property bool searchOpen: false
    property bool suspended: false

    // suspendToTray: drop every thumbnail while hidden; restoreFromTray reloads
    onSuspendedChanged: {
        if (suspended) {
            pageItems = []
            prefetch = []
        } else {
            rebuild()
        }
    }

    function itemsOf(p) {
        var start = p * pageSize
        var out = []
        var n = Math.min(pageSize, Math.max(0, Library.count - start))
        for (var i = 0; i < n; ++i)
            out.push(Library.at(start + i))
        return out
    }

    function setCursor(i) {
        cursor = Math.max(0, Math.min(i, pageItems.length - 1))
        if (pageItems.length > 0)
            selectedId = pageItems[cursor].id
    }

    /// WallpaperBrowser.loadPage: clamp the page, fill it, preload the pages
    /// around it, and put the selection back on the same wallpaper if it is
    /// on this page (else the first cell).
    function rebuild() {
        page = Math.max(0, Math.min(page, pageCount - 1))
        var items = itemsOf(page)
        pageItems = items
        pageLabel = (page + 1) + "/" + pageCount

        var restored = 0
        for (var i = 0; i < items.length; ++i) {
            if (items[i].id === selectedId) { restored = i; break }
        }
        setCursor(restored)

        var around = []
        for (const p of [page - 1, page + 1]) {
            if (p >= 0 && p < pageCount && p !== page)
                for (const item of itemsOf(p))
                    if (item.preview !== "") around.push(item.preview)
        }
        prefetch = around
    }

    function goTo(newPage, newCursor) {
        if (newPage < 0 || newPage >= pageCount)
            return
        flipDir = newPage > page ? 1 : -1
        page = newPage
        deck.x = flipDir * 42
        deck.opacity = 0.3
        rebuild()
        if (newCursor !== undefined)
            setCursor(newCursor)       // moveSelection across pages picks the cell
        deckAnim.restart()
    }

    /// WallpaperBrowser.moveSelection
    function move(dx, dy) {
        if (pageItems.length === 0)
            return
        var col = cursor % cols
        var row = Math.floor(cursor / cols)
        if (dy < 0 && row === 0) {
            if (page > 0) goTo(page - 1, (rows - 1) * cols + col)
            return
        }
        if (dy > 0 && row === rows - 1) {
            if (page < pageCount - 1) goTo(page + 1, col)
            return
        }
        var next = cursor + dy * cols + dx
        if (next >= 0 && next < pageItems.length)
            setCursor(next)
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

    Timer {
        id: resizeDebounce
        interval: 150
        onTriggered: {
            view.cols = view.liveCols
            view.rows = view.liveRows
        }
    }
    onLiveColsChanged: resizeDebounce.restart()
    onLiveRowsChanged: resizeDebounce.restart()

    Component.onCompleted: {
        cols = liveCols
        rows = liveRows
        rebuild()
    }
    onVisibleChanged: if (visible) rebuild()
    onPageSizeChanged: rebuild()       // same page index, selection by id

    Connections {
        target: Library
        function onFilterChanged() {
            view.page = 0
            view.rebuild()
        }
        function onCountChanged() { view.rebuild() }   // onLibraryChange
    }

    // preloadAdjacentPages: decode the neighbours' thumbnails into the cache
    Repeater {
        model: view.prefetch
        delegate: Image {
            required property string modelData
            visible: false
            source: modelData
            asynchronous: true
            cache: true
            sourceSize.width: 360
            sourceSize.height: 360
        }
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
                        view.setCursor(index)
                        view.openCurrent()
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
                    view.openCurrent()
                }
            }
            Keys.onDownPressed: view.closeSearch(false)
        }
    }
}
