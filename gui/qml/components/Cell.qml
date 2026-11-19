import QtQuick
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: cell

    required property string preview
    required property int index

    property int pageStart: 0
    property bool selected: false
    property bool hovered: mouse.containsMouse

    property real sel: selected ? 1 : 0
    property real hot: hovered ? 1 : 0

    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))
    readonly property int radius: 6

    signal clicked

    transform: Scale {
        origin.x: cell.width / 2
        origin.y: cell.height / 2
        xScale: 1.0 + 0.02 * cell.hot
        yScale: 1.0 + 0.02 * cell.hot
        Behavior on xScale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
        Behavior on yScale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
    }
    Behavior on sel { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
    Behavior on hot { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

    // Shader-free shadow: two offset slabs behind the card.
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 5 + 2 * cell.hot
        anchors.bottomMargin: -3
        anchors.leftMargin: -2
        anchors.rightMargin: -2
        radius: cell.radius + 2
        color: "#000000"
        opacity: 0.28 + 0.12 * cell.hot
    }
    Rectangle {
        anchors.fill: parent
        anchors.topMargin: 2
        anchors.bottomMargin: -1
        radius: cell.radius + 1
        color: "#000000"
        opacity: 0.22
    }

    Rectangle {
        id: body
        anchors.fill: parent
        radius: cell.radius
        color: "#151A21"
        clip: true

        Rectangle {
            anchors.fill: parent
            visible: cell.preview === ""
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.hsla(((cell.pageStart + cell.index) * 0.11) % 1.0, 0.55, 0.40, 1.0) }
                GradientStop { position: 1.0; color: Qt.hsla(((cell.pageStart + cell.index) * 0.11 + 0.1) % 1.0, 0.6, 0.14, 1.0) }
            }
        }

        AnimatedImage {
            anchors.fill: parent
            anchors.margins: 4
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
            playing: cell.animate
            sourceSize.width: 360
            sourceSize.height: 360
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 1
        radius: cell.radius - 1
        height: 1
        color: "#14ffffff"
    }

    Rectangle {
        anchors.fill: parent
        radius: cell.radius
        color: "transparent"
        border.width: 1
        border.color: cell.selected ? Theme.accent : "#2A323D"
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -2
        radius: cell.radius + 2
        color: "transparent"
        border.width: 2
        border.color: Theme.accent
        opacity: 0.85 * cell.sel
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
