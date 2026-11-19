import QtQuick
import QtQuick.Controls
import NyaaPaper.Theme 1.0

/// A button that keeps its own chrome, independent of the control style.
Button {
    id: button

    property bool primary: false

    implicitHeight: 34
    implicitWidth: Math.max(96, contentLabel.implicitWidth + 30)
    font.pixelSize: 12

    contentItem: Text {
        id: contentLabel
        text: button.text
        color: button.primary ? Theme.accent : Theme.text
        font: button.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: button.enabled ? 1.0 : 0.4
    }

    background: Rectangle {
        radius: 7
        color: button.down ? Theme.accentDim
                           : (button.hovered ? Theme.panelAlt : "#1A1F27")
        border.width: 1
        border.color: button.primary ? Theme.accent : Theme.line
        opacity: button.enabled ? 1.0 : 0.5
    }
}
