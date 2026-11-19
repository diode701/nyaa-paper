import QtQuick
import QtQuick.Layouts

/// The box every setting's control sits on.
/// Java: WallpaperPropertiesView.createSettingBox (and GroupProperty's #333).
Rectangle {
    id: card
    default property alias content: inner.data
    property bool group: false

    Layout.fillWidth: true
    Layout.leftMargin: 20
    Layout.rightMargin: 20
    implicitHeight: inner.implicitHeight + 20
    radius: 5
    color: group ? "#333333" : "#2a2a2a"

    ColumnLayout {
        id: inner
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: 10
        spacing: 5
    }
}
