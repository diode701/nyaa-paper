import QtQuick
import QtQuick.Effects
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

Item {
    id: cell

    required property string title
    required property string kind
    required property string preview
    required property int seed

    property bool selected: false
    property bool hovered: mouse.containsMouse

    readonly property bool animate: Settings.animationMode === 2
                                    || (Settings.animationMode === 1 && (hovered || selected))

    signal clicked

    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radius
        border.width: cell.selected ? 2 : 1
        border.color: cell.selected ? Theme.accent : "#2B333F"
        clip: true

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#171C24" }
            GradientStop { position: 1.0; color: "#0E1218" }
        }

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: cell.selected ? 0.75 : 0.55
            shadowBlur: cell.selected ? 0.7 : 0.5
            shadowVerticalOffset: cell.selected ? 8 : 5
            shadowHorizontalOffset: 0
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
            anchors.margins: 1
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
            playing: cell.animate
            sourceSize.width: 400
            sourceSize.height: 400
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 52
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#e6070809" }
            }
        }

        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: 1
            color: "#1Affffff"
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 9
            text: cell.title
            color: Theme.text
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Rectangle {
            visible: cell.kind !== ""
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 9
            implicitWidth: kindLabel.implicitWidth + 12
            implicitHeight: 18
            radius: 4
            color: "#99070809"
            Text {
                id: kindLabel
                anchors.centerIn: parent
                text: cell.kind.toUpperCase()
                color: Theme.muted
                font.pixelSize: 8
                font.letterSpacing: 1.4
            }
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
