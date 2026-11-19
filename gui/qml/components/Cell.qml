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
    readonly property int radius: Look.cardRadius

    signal clicked

    Behavior on sel { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
    Behavior on hot { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }

    transform: Scale {
        origin.x: cell.width / 2
        origin.y: cell.height / 2
        xScale: 1.0 + Look.hoverScale * cell.hot
        yScale: 1.0 + Look.hoverScale * cell.hot
    }

    // ---- card body ---------------------------------------------------------
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
    }

    // Shadow + glow come from the body, so the preview is not part of the shape.
    MultiEffect {
        source: body
        anchors.fill: body
        autoPaddingEnabled: true
        blurMax: cell.selected ? Look.blurMaxSelected : Look.blurMax
        shadowEnabled: true
        shadowColor: Qt.rgba(0.30 * cell.sel, 0.10 + 0.72 * cell.sel, 0.14 + 0.86 * cell.sel, 1.0)
        shadowOpacity: Look.shadowOpacity + Look.shadowOpacityHover * cell.hot
                        + (Look.shadowOpacitySelected - Look.shadowOpacity) * cell.sel
        shadowBlur: cell.selected ? Look.shadowBlurSelected : Look.shadowBlur
        shadowVerticalOffset: Look.shadowOffset * (1.0 - cell.sel)
        shadowHorizontalOffset: 0
    }

    // ---- preview, masked to the card shape ---------------------------------
    // When masking is on the image itself must be hidden, otherwise its square
    // corners stay visible underneath the masked copy. If a backend cannot run
    // the effect the preview disappears, which is obvious and one Look toggle
    // away from the plain path.
    AnimatedImage {
        id: previewImage
        anchors.fill: parent
        anchors.margins: Look.imageInset
        source: cell.preview
        visible: cell.preview !== "" && !Look.roundPreviews
        fillMode: Image.PreserveAspectFit
        asynchronous: true
        cache: false
        playing: cell.animate
        sourceSize.width: 360
        sourceSize.height: 360
    }

    Rectangle {
        id: maskShape
        anchors.fill: parent
        anchors.margins: Look.imageInset
        radius: Math.max(0, cell.radius - Look.imageInset)
        color: "black"
        visible: false
    }

    MultiEffect {
        anchors.fill: previewImage
        source: previewImage
        visible: Look.roundPreviews && cell.preview !== ""
        maskEnabled: Look.roundPreviews
        maskSource: maskShape
    }

    // top bevel
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 1
        height: 1
        color: "#1fffffff"
    }

    Rectangle {
        anchors.fill: parent
        radius: cell.radius
        color: "transparent"
        border.width: cell.selected ? Look.borderWidthSelected : Look.borderWidth
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
