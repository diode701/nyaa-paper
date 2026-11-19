import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// ComboSetting → label above, choice below (ComboProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var display      // the label of the current key
    required property var options

    Text { text: label; color: "#d3d3d3"; font.pixelSize: 12 }
    ComboBox {
        Layout.preferredWidth: 240
        implicitHeight: 30
        font.pixelSize: 12
        model: options
        currentIndex: Math.max(0, options.indexOf(String(display)))
        onActivated: Properties.setValue(name, currentText)
        background: Rectangle { color: "#444444"; radius: 3; border.color: "#555555" }
    }
}
