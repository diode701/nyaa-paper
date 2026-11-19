import QtQuick
import QtQuick.Controls
import NyaaPaper.Theme 1.0

/// A thin track and a small square handle.
Slider {
    id: slider
    implicitWidth: 260
    implicitHeight: 22
    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        width: slider.availableWidth
        height: 3
        radius: 1
        color: Theme.line
        Rectangle { width: slider.visualPosition * parent.width; height: parent.height; color: slider.enabled ? Theme.accent : Theme.dim }
    }
    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
        y: slider.topPadding + slider.availableHeight / 2 - height / 2
        width: 12; height: 12; radius: 1
        color: slider.enabled ? "#ffffff" : Theme.muted
        border.color: slider.pressed || slider.activeFocus ? Theme.accent : "transparent"
        border.width: 2
    }
}
