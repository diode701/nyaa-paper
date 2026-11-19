import QtQuick
import NyaaPaper.Theme 1.0

/// A number with − and +. Arrow keys step, typing edits.
Rectangle {
    id: stepper
    property int value: 0
    property int from: 0
    property int to: 100
    property int step: 1
    property string suffix: ""
    signal modified(int value)
    implicitWidth: 120
    implicitHeight: 28
    radius: Theme.radius
    color: Theme.bg
    border.color: Theme.line
    opacity: enabled ? 1 : 0.45

    function set(v) {
        const clamped = Math.max(from, Math.min(to, v))
        if (clamped !== value) { value = clamped; modified(clamped) }
    }

    Text {
        anchors.left: parent.left; anchors.leftMargin: 9
        anchors.verticalCenter: parent.verticalCenter
        text: "−"; color: Theme.muted; font.family: Theme.font; font.pixelSize: 14
        MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: stepper.set(stepper.value - stepper.step) }
    }
    TextInput {
        anchors.centerIn: parent
        width: parent.width - 52
        horizontalAlignment: TextInput.AlignHCenter
        text: stepper.value + stepper.suffix
        color: Theme.text
        font.family: Theme.font
        font.pixelSize: 12
        validator: RegularExpressionValidator { regularExpression: /-?\d*.*/ }
        onEditingFinished: stepper.set(parseInt(text))
        Keys.onUpPressed: stepper.set(stepper.value + stepper.step)
        Keys.onDownPressed: stepper.set(stepper.value - stepper.step)
    }
    Text {
        anchors.right: parent.right; anchors.rightMargin: 9
        anchors.verticalCenter: parent.verticalCenter
        text: "+"; color: Theme.muted; font.family: Theme.font; font.pixelSize: 14
        MouseArea { anchors.fill: parent; anchors.margins: -6; onClicked: stepper.set(stepper.value + stepper.step) }
    }
}
