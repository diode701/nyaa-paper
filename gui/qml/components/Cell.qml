import QtQuick
import QtQuick.Window
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

/// One grid cell. Java: WallpaperBrowser.createEmptyCell + loadPage/setCellImage.
///
/// The browser keeps a fixed pool of these and only swaps `item`, so a page
/// flip moves data, not objects. The shadow is one pre-rendered image shared
/// by every card (no per-card blur pass). While the thumbnail loads the short
/// title shows (Java's placeholder), then the image fades in over 300 ms.
Item {
    id: cell

    property var item: null            // {id, title, kind, preview, basePath}
    property bool selected: false
    property string hoverStyle: Config.hoverStyle          // java | lift | off
    property string selectionStyle: Config.selectionStyle  // brackets | outline | quiet

    readonly property bool hovered: mouse.containsMouse
    readonly property string preview: item ? item.preview : ""
    readonly property string thumb: item ? (item.thumb || "") : ""
    readonly property int edge: Math.round(width * Screen.devicePixelRatio)
    readonly property string title: item ? item.title : ""
    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))
    readonly property bool isGif: preview.toLowerCase().endsWith(".gif")
    readonly property bool imageReady: still.status === Image.Ready

    signal clicked

    visible: item !== null

    /// WallpaperBrowser.shortTitle
    function shortTitle(text) { return text.length > 20 ? text.substring(0, 18) + "…" : text }

    // hover: Java = 2 % zoom and a light accent border; lift = rise 3 px
    property real hot: hovered && hoverStyle !== "off" ? 1 : 0
    Behavior on hot { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    property real sel: selected ? 1 : 0
    Behavior on sel { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    opacity: selectionStyle === "quiet" && !selected ? 0.62 : 1
    Behavior on opacity { NumberAnimation { duration: 200 } }

    transform: [
        Scale {
            origin.x: cell.width / 2
            origin.y: cell.height / 2
            xScale: 1 + (cell.hoverStyle === "java" ? 0.02 : 0) * cell.hot
            yScale: xScale
        },
        Translate {
            y: -3 * (cell.hoverStyle === "lift" ? cell.hot : 0)
               - 2 * (cell.selectionStyle !== "quiet" ? cell.sel : 0)
        }
    ]

    // shared shadow texture, 20 px of soft edge around the card
    BorderImage {
        anchors.fill: parent
        anchors.margins: -20
        anchors.topMargin: -16
        anchors.bottomMargin: -24
        source: "qrc:/card-shadow.png"
        border { left: 40; top: 40; right: 40; bottom: 40 }
        opacity: 0.85 + 0.15 * cell.sel
    }

    Rectangle {
        id: body
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.card
        clip: true

        // placeholder: the short title while the thumbnail loads
        Text {
            anchors.centerIn: parent
            width: parent.width - 16
            visible: !cell.imageReady
            text: cell.shortTitle(cell.title)
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 11
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        // the still thumbnail, from the app's own cache (C++, centre-cropped,
        // decoded at exactly this card's pixel size; prefetched pages are instant)
        Image {
            id: still
            anchors.fill: parent
            anchors.margins: 1
            source: cell.thumb
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: false            // ThumbnailCache already holds it
            sourceSize.width: cell.edge
            sourceSize.height: cell.edge
            visible: !animated.showing
            opacity: cell.imageReady ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 300 } }
        }

        // the GIF decoder runs only while it may play
        Loader {
            id: animated
            anchors.fill: still
            active: cell.isGif && cell.animate && cell.visible
            readonly property bool showing: status === Loader.Ready && item && item.status === AnimatedImage.Ready
            sourceComponent: AnimatedImage {
                source: cell.preview
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: false
                playing: true
                sourceSize.width: 360
                sourceSize.height: 360
            }
        }

        // title and type, on the selected card only
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: caption.implicitHeight + 30
            opacity: cell.sel
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: "#d0000000" }
            }
            Column {
                id: caption
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 9
                spacing: 2
                Text {
                    width: parent.width
                    text: cell.title
                    color: "#ffffff"
                    font.family: Theme.font
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                    maximumLineCount: 2
                    elide: Text.ElideRight
                }
                Text {
                    text: (cell.item ? (cell.item.kind || "") : "").toUpperCase()
                    color: "#b0ffffff"
                    font.family: Theme.font
                    font.pixelSize: 9
                    font.letterSpacing: 1
                }
            }
        }
    }

    // border: hover accent (Java), selection outline
    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: "transparent"
        border.width: cell.selected && cell.selectionStyle === "outline" ? 2 : 1
        border.color: cell.selected
                      ? (cell.selectionStyle === "quiet" ? Qt.lighter(Theme.line, 1.9) : Theme.accent)
                      : (cell.hot > 0.5 && cell.hoverStyle === "java" ? Theme.accent2 : Theme.line)
        Behavior on border.color { ColorAnimation { duration: 120 } }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
