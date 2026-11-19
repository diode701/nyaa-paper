import QtQuick
import QtQuick.Layouts
import NyaaPaper.Backend 1.0
import "../flat"

/// ComboSetting (ComboProperty.createControl): segmented when it fits,
/// a dropdown when the list is long
SettingCard {
    id: control
    required property string name
    required property string label
    required property var display      // label of the current key
    required property var options
    caption: label
    readonly property bool shortList: options.length <= 4 && options.join("").length <= 40
    Segmented {
        visible: control.shortList
        options: control.options
        value: String(control.display)
        upper: false
        onPicked: v => Properties.setValue(control.name, v)
    }
    FlatCombo {
        visible: !control.shortList
        model: control.options
        currentIndex: Math.max(0, control.options.indexOf(String(control.display)))
        onActivated: Properties.setValue(control.name, currentText)
    }
    Item { Layout.fillWidth: true }
}
