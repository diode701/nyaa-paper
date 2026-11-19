import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import NyaaPaper.Backend 1.0

/// ScalingOption → heading and a combo (ScalingSetting.createControl)
SettingCard {
    required property string name
    required property string label
    required property var settingValue
    required property var options

    Text { text: label; color: "#e0e0e0"; font.bold: true; font.pixelSize: 12 }
    ComboBox {
        Layout.preferredWidth: 240
        implicitHeight: 30
        font.pixelSize: 12
        model: options
        currentIndex: Math.max(0, options.indexOf(String(settingValue)))
        onActivated: GlobalOptions.setValue(name, currentText)
    }
}
