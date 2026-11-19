import QtQuick
import QtQuick.Effects
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
    readonly property int radius: 3

    signal clicked

    Behavior on sel { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    Behavior on hot { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    transform: Scale {
        origin.x: cell.width / 2
        origin.y: cell.height / 2
        xScale: 1.0 + 0.025 * cell.hot
        yScale: 1.0 + 0.025 * cell.hot
    }

    Rectangle {
        id: body
        anchors.fill: parent
        anchors.topMargin: -2 * cell.hot
        radius: cell.radius
        clip: true

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#252E3A" }
            GradientStop { position: 1.0; color: "#191E26" }
        }

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
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
            playing: cell.animate
            sourceSize.width: 360
            sourceSize.height: 360
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            color: "#1fffffff"
        }
    }

    // Real gaussian-ish shadow, and the accent glow when selected. The padding
    // is what lets it bleed outside the card; without it the layer clips it.
    MultiEffect {
        source: body
        anchors.fill: body
        autoPaddingEnabled: true
        blurMax: cell.selected ? 18 : 14
        shadowEnabled: true
        shadowColor: Qt.rgba(0.30 * cell.sel, 0.10 + 0.72 * cell.sel, 0.14 + 0.86 * cell.sel, 1.0)
        shadowOpacity: 0.40 + 0.10 * cell.hot + 0.22 * cell.sel
        shadowBlur: 0.45 + 0.28 * cell.sel
        shadowVerticalOffset: 3.0 * (1.0 - cell.sel)
        shadowHorizontalOffset: 0
    }

    Rectangle {
        anchors.fill: parent
        radius: cell.radius
        color: "transparent"
        border.width: cell.selected ? 3 : 1
        border.color: cell.selected ? Theme.accent : (cell.hot > 0.2 ? "#46525F" : "#333C49")
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 3
        radius: cell.radius
        color: "transparent"
        border.width: 1
        border.color: "#66ffffff"
        opacity: 0.35 * cell.sel
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
