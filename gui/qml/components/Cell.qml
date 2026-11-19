import QtQuick
import QtQuick.Effects
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

/// One grid cell: #2a2a2a, radius 8, border #3a3a3a; selected 3px #4a90e2 with
/// a blue glow; hover #6aa9ff 2px and scale 1.02. Image 170x170, fade 300 ms.
Item {
    id: cell

    property string preview: ""
    property string title: ""

    property int pageStart: 0
    property bool selected: false
    property bool hovered: mouse.containsMouse
    property real sel: selected ? 1 : 0
    property real hot: hovered ? 1 : 0

    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))
    readonly property int radius: Look.cardRadius

    signal clicked

    transform: Scale {
        origin.x: cell.width / 2
        origin.y: cell.height / 2
        xScale: 1.0 + Look.hoverScale * cell.hot
        yScale: 1.0 + Look.hoverScale * cell.hot
    }

    Rectangle {
        id: body
        anchors.fill: parent
        radius: cell.radius
        color: "#2a2a2a"
        border.width: cell.selected ? 3 : (cell.hot > 0.5 ? 2 : 1)
        border.color: cell.selected ? Theme.accent
                                    : (cell.hot > 0.5 ? Theme.accentHover : "#3a3a3a")
        clip: true

        AnimatedImage {
            id: image
            anchors.centerIn: parent
            width: 170
            height: 170
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
            playing: cell.animate
            sourceSize.width: 340
            sourceSize.height: 340

            opacity: 0
            Behavior on opacity { NumberAnimation { duration: 300 } }
            onStatusChanged: if (status === Image.Ready) opacity = 1
        }

        Text {
            anchors.centerIn: parent
            width: 170
            visible: cell.preview === ""
            text: cell.title.length > 20 ? cell.title.substring(0, 18) + "…" : cell.title
            color: "#d3d3d3"
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
    }

    MultiEffect {
        source: body
        anchors.fill: body
        autoPaddingEnabled: true
        blurMax: cell.selected ? Look.blurMaxSelected : Look.blurMax
        shadowEnabled: true
        shadowColor: cell.selected ? Theme.accent : "#000000"
        shadowOpacity: cell.selected ? 0.65 : 0.5
        shadowBlur: cell.selected ? Look.shadowBlurSelected : Look.shadowBlur
        shadowVerticalOffset: 2
        shadowHorizontalOffset: 0
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
