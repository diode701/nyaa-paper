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
    readonly property int radius: 7

    signal clicked

    Behavior on sel { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }
    Behavior on hot { NumberAnimation { duration: 140; easing.type: Easing.OutCubic } }

    transform: Scale {
        origin.x: cell.width / 2
        origin.y: cell.height / 2
        xScale: 1.0 + 0.025 * cell.hot
        yScale: 1.0 + 0.025 * cell.hot
    }

    // ---- contact shadow: stacked rings, widest and faintest first ----------
    Rectangle { anchors.fill: parent; anchors.margins: -10; anchors.topMargin: 14; radius: cell.radius + 10; color: "#000000"; opacity: 0.06 }
    Rectangle { anchors.fill: parent; anchors.margins: -7;  anchors.topMargin: 11; radius: cell.radius + 7;  color: "#000000"; opacity: 0.09 }
    Rectangle { anchors.fill: parent; anchors.margins: -4;  anchors.topMargin: 8;  radius: cell.radius + 4;  color: "#000000"; opacity: 0.13 }
    Rectangle { anchors.fill: parent; anchors.margins: -2;  anchors.topMargin: 5;  radius: cell.radius + 2;  color: "#000000"; opacity: 0.18 }

    // ---- accent glow, only on the cursor -----------------------------------
    Rectangle { anchors.fill: parent; anchors.margins: -11; radius: cell.radius + 11; color: "transparent"; border.width: 1; border.color: Theme.accent; opacity: 0.06 * cell.sel }
    Rectangle { anchors.fill: parent; anchors.margins: -7;  radius: cell.radius + 7;  color: "transparent"; border.width: 1; border.color: Theme.accent; opacity: 0.13 * cell.sel }
    Rectangle { anchors.fill: parent; anchors.margins: -4;  radius: cell.radius + 4;  color: "transparent"; border.width: 2; border.color: Theme.accent; opacity: 0.26 * cell.sel }

    // ---- the card ----------------------------------------------------------
    Rectangle {
        id: body
        anchors.fill: parent
        anchors.topMargin: -1 * cell.hot
        radius: cell.radius
        clip: true

        gradient: Gradient {
            GradientStop { position: 0.0; color: "#2C3542" }
            GradientStop { position: 1.0; color: "#1C222B" }
        }

        border.width: cell.selected ? 2 : 1
        border.color: cell.selected ? Theme.accent : (cell.hot > 0.2 ? "#46525F" : "#3A4553")

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
            anchors.margins: 2
            source: cell.preview
            visible: cell.preview !== ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: false
            playing: cell.animate
            sourceSize.width: 360
            sourceSize.height: 360
        }

        // top bevel highlight + bottom shade, the cheap "3d" trick
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: 1
            color: "#26ffffff"
        }
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            height: 1
            color: "#40000000"
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        onClicked: cell.clicked()
    }
}
