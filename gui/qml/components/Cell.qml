import QtQuick
import QtQuick.Effects
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: cell

    required property string preview
    required property int seed

    property bool selected: false
    property bool hovered: mouse.containsMouse

    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))

    signal clicked

    Rectangle {
        id: body
        anchors.fill: parent
        radius: Theme.radius
        color: "#151A21"
        clip: true

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: cell.selected ? Theme.accent : "#000000"
            shadowOpacity: cell.selected ? 0.55 : 0.45
            shadowBlur: cell.selected ? 1.0 : 0.4
            shadowVerticalOffset: cell.selected ? 0 : 4
        }

        Rectangle {
            anchors.fill: parent
            visible: cell.preview === ""
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.hsla((cell.seed * 0.11) % 1.0, 0.55, 0.40, 1.0) }
                GradientStop { position: 1.0; color: Qt.hsla((cell.seed * 0.11 + 0.1) % 1.0, 0.6, 0.14, 1.0) }
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
        border.width: cell.selected ? 3 : 1
        border.color: cell.selected ? Theme.accent : "#2A323D"
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
