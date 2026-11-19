import QtQuick
import QtQuick.Controls
import NyaaPaper.Theme 1.0

/// A dropdown for lists too long for Segmented.
ComboBox {
    id: combo
    implicitWidth: 240
    implicitHeight: 28
    font.family: Theme.font
    font.pixelSize: 12

    background: Rectangle {
        radius: Theme.radius
        color: Theme.raise
        border.color: combo.activeFocus || combo.popup.visible ? Theme.accent : Theme.line
    }
    contentItem: Text {
        leftPadding: 10
        rightPadding: 24
        text: combo.displayText
        color: Theme.text
        font: combo.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Text {
        x: combo.width - width - 10
        anchors.verticalCenter: parent.verticalCenter
        text: "▾"
        color: Theme.muted
        font.pixelSize: 11
    }
    delegate: ItemDelegate {
        required property int index
        required property var modelData
        width: combo.width
        height: 28
        contentItem: Text {
            text: typeof modelData === "string" ? modelData : (modelData[combo.textRole] || "")
            color: index === combo.currentIndex ? Theme.accent : Theme.text
            font: combo.font
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: parent.highlighted ? Theme.raise : Theme.panel }
        highlighted: combo.highlightedIndex === index
    }
    popup: Popup {
        y: combo.height + 2
        width: combo.width
        padding: 1
        implicitHeight: Math.min(contentItem.implicitHeight + 2, 320)
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: combo.popup.visible ? combo.delegateModel : null
            currentIndex: combo.highlightedIndex
        }
        background: Rectangle { color: Theme.panel; border.color: Theme.line; radius: Theme.radius }
    }
}
