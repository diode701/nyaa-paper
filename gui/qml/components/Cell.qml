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

    // Animated 0..1 levels so selection and hover ease instead of snapping.
    property real sel: selected ? 1 : 0
    property real hot: hovered ? 1 : 0

    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))

    signal clicked

    scale: 1.0 + 0.02 * hot
    Behavior on sel { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
    Behavior on hot { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }

    Rectangle {
        id: body
        anchors.fill: parent
        radius: Theme.radius
        color: "#151A21"
        clip: true

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Qt.rgba(0.35 * cell.sel, 0.16 + 0.66 * cell.sel, 0.20 + 0.80 * cell.sel, 1.0)
            shadowOpacity: 0.45 + 0.15 * cell.hot + 0.30 * cell.sel
            shadowBlur: 0.4 + 0.5 * cell.hot + 0.6 * cell.sel
            shadowVerticalOffset: 4 * (1.0 - cell.sel)
            shadowHorizontalOffset: 0
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
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: "transparent"
        border.width: 1
        border.color: "#2A323D"
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius
        color: "transparent"
        border.width: 3
        border.color: Theme.accent
        opacity: cell.sel
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: -1
        radius: Theme.radius + 1
        color: "transparent"
        border.width: 1
        border.color: Theme.accent
        opacity: 0.35 * cell.sel
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
