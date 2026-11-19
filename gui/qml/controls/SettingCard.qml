import QtQuick
import QtQuick.Layouts
import NyaaPaper.Theme 1.0

/// One setting as a row: its caption on the left, its control on the right.
/// Java: WallpaperPropertiesView.createSettingBox. Groups and text labels
/// span the row instead.
Rectangle {
    id: card
    default property alias content: slot.data
    property string caption: ""
    property bool group: false

    Layout.fillWidth: true
    Layout.leftMargin: 20
    Layout.rightMargin: 20
    implicitHeight: Math.max(group ? 30 : 42, row.implicitHeight + 14)
    radius: Theme.radius
    color: group ? "transparent" : (hover.hovered ? Qt.lighter(Theme.card, 1.08) : Theme.card)

    HoverHandler { id: hover }

    RowLayout {
        id: row
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: card.group ? 2 : 14
        anchors.rightMargin: 14
        spacing: 16
        Text {
            visible: card.caption !== ""
            text: card.caption
            color: Theme.muted
            font.family: Theme.font
            font.pixelSize: 12
            wrapMode: Text.WordWrap
            Layout.preferredWidth: 230
            Layout.maximumWidth: 230
        }
        RowLayout {
            id: slot
            Layout.fillWidth: true
            spacing: 10
        }
    }
}
