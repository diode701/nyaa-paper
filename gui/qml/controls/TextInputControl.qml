import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../flat"

/// TextInputSetting (TextInputProperty.createControl)
SettingCard {
    required property string name
    required property string label
    required property var display
    caption: label
    FlatField { Layout.preferredWidth: 400; text: String(display); onEdited: t => Properties.setValue(name, t) }
    Item { Layout.fillWidth: true }
}
