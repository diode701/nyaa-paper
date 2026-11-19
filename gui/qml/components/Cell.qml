import QtQuick
import NyaaPaper.Theme 1.0

Item {
    id: cell

    required property string title
    required property string kind
    required property string preview
    required property int index

    property bool selected: false
    property bool hovered: mouse.containsMouse
    signal clicked


    Rectangle {
        id: card
        anchors.fill: parent
        radius: Theme.radius
        color: Theme.panel
        border.width: 1
        border.color: cell.hovered ? Theme.dim : Theme.line
        clip: true

        Rectangle {
            anchors.fill: parent
            visible: cell.preview === ""
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.hsla((cell.index * 0.11) % 1.0, 0.55, 0.42, 1.0) }
                GradientStop { position: 1.0; color: Qt.hsla((cell.index * 0.11 + 0.1) % 1.0, 0.6, 0.16, 1.0) }
            }
        }

        Image {
            anchors.fill: parent
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            sourceSize.width: 480
            sourceSize.height: 320
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 72
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#00000000" }
                GradientStop { position: 1.0; color: "#e6070809" }
            }
        }

        Text {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.bottomMargin: 12
            text: cell.title
            color: Theme.text
            font.pixelSize: 13
            elide: Text.ElideRight
        }

        Rectangle {
            visible: cell.kind !== ""
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.margins: 10
            implicitWidth: kindLabel.implicitWidth + 14
            implicitHeight: 20
            radius: 4
            color: "#99070809"
            Text {
                id: kindLabel
                anchors.centerIn: parent
                text: cell.kind.toUpperCase()
                color: Theme.muted
                font.pixelSize: 9
                font.letterSpacing: 1.5
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
