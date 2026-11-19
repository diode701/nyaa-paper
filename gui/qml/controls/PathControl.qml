import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../components"
import "../flat"

/// What File, Directory and Texture share: a path field and Browse…
SettingCard {
    id: control
    required property string name
    required property string label
    required property var display
    property bool chooseFolder: false
    property string dialogTitle: "Select file for " + label
    caption: label

    function pathOf(url) { return decodeURIComponent(url.toString().replace(/^file:\/\//, "")) }

    FlatField { Layout.preferredWidth: 360; text: String(control.display); onEdited: t => Properties.setValue(control.name, t) }
    ActionButton { text: "Browse…"; onClicked: control.chooseFolder ? folderDialog.open() : fileDialog.open() }
    Item { Layout.fillWidth: true }
    FileDialog { id: fileDialog; title: control.dialogTitle; onAccepted: Properties.setValue(control.name, control.pathOf(selectedFile)) }
    FolderDialog { id: folderDialog; title: control.dialogTitle; onAccepted: Properties.setValue(control.name, control.pathOf(selectedFolder)) }
}
