import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../components"

/// The shape File, Directory and Texture share: label, path field, Browse…
/// Each of those three classes has its own control that sets these two.
SettingCard {
    id: control
    required property string name
    required property string label
    required property var display

    property bool chooseFolder: false
    property string dialogTitle: "Select file for " + label

    function pathOf(url) { return decodeURIComponent(url.toString().replace(/^file:\/\//, "")) }

    Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
    RowLayout {
        spacing: 8
        TextField {
            Layout.preferredWidth: 350        // Java: setMaxWidth(350)
            text: String(display)
            color: "#ffffff"
            font.pixelSize: 12
            selectByMouse: true
            onEditingFinished: Properties.setValue(control.name, text)
        }
        ActionButton {
            text: "Browse..."
            onClicked: control.chooseFolder ? folderDialog.open() : fileDialog.open()
        }
        Item { Layout.fillWidth: true }
    }
    FileDialog {
        id: fileDialog
        title: control.dialogTitle
        onAccepted: Properties.setValue(control.name, control.pathOf(selectedFile))
    }
    FolderDialog {
        id: folderDialog
        title: control.dialogTitle
        onAccepted: Properties.setValue(control.name, control.pathOf(selectedFolder))
    }
}
