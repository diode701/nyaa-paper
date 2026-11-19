import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// TextInputSetting → label and a text field (TextInputProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var display

    Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
    TextField {
        Layout.preferredWidth: 400        // Java: setMaxWidth(400)
        text: String(display)
        color: "#ffffff"
        font.pixelSize: 12
        selectByMouse: true
        onEditingFinished: Properties.setValue(name, text)
    }
}
