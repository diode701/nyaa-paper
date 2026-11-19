import QtQuick
import NyaaPaper.Backend 1.0
import NyaaPaper.Theme 1.0

/// Your monitors as they are arranged, scaled to fit: pick where to apply.
/// Positions and sizes come from Qt's screen list; names added by hand in
/// Settings (outputs Qt does not report) sit to the right at 1920×1080.
/// A running monitor shows its wallpaper; the chosen one is outlined.
Item {
    id: map
    property var monitors: Engine.monitors
    property string selected: ""
    property int revision: 0            // bump to refresh the thumbnails
    signal picked(string monitor)

    implicitHeight: 118

    readonly property var layout: {
        const screens = Qt.application.screens
        const placed = []
        let right = 0
        for (let i = 0; i < monitors.length; ++i) {
            const name = monitors[i]
            let found = null
            for (let k = 0; k < screens.length; ++k)
                if (screens[k].name === name) found = screens[k]
            if (found) {
                placed.push({ name: name, index: i, x: found.virtualX, y: found.virtualY, w: found.width, h: found.height })
                right = Math.max(right, found.virtualX + found.width)
            }
        }
        for (let i = 0; i < monitors.length; ++i) {
            const name = monitors[i]
            if (!placed.some(p => p.name === name)) {
                placed.push({ name: name, index: i, x: right + 60, y: 0, w: 1920, h: 1080 })
                right += 1980
            }
        }
        if (placed.length === 0)
            return { scale: 1, offsetX: 0, offsetY: 0, rects: [] }
        const minX = Math.min(...placed.map(p => p.x)), minY = Math.min(...placed.map(p => p.y))
        const maxX = Math.max(...placed.map(p => p.x + p.w)), maxY = Math.max(...placed.map(p => p.y + p.h))
        const pad = 4   // room for the selection outline
        const scale = Math.min((width - 2 * pad) / (maxX - minX), (height - 2 * pad) / (maxY - minY))
        return {
            rects: placed.map(p => ({ name: p.name, index: p.index,
                                      x: pad + (p.x - minX) * scale, y: pad + (p.y - minY) * scale,
                                      w: Math.max(24, p.w * scale - 4), h: Math.max(16, p.h * scale - 4) }))
        }
    }

    Repeater {
        model: map.layout.rects
        delegate: Item {
            id: screen
            required property var modelData
            readonly property bool chosen: modelData.name === map.selected
            readonly property bool live: Engine.running[modelData.name] !== undefined
            readonly property string preview: { map.revision; return live ? Controller.runningPreview(modelData.name) : "" }
            x: modelData.x; y: modelData.y; width: modelData.w; height: modelData.h

            // the outline sits 2 px outside the screen
            Rectangle {
                anchors.fill: parent
                anchors.margins: -3
                color: "transparent"
                radius: Theme.radius
                border.color: Theme.accent
                visible: screen.chosen
            }
            Rectangle {
                anchors.fill: parent
                radius: Theme.radius
                color: Theme.bg
                border.color: screen.chosen ? Theme.accent : Theme.line
                clip: true
                Image {
                    anchors.fill: parent
                    anchors.margins: 1
                    source: screen.preview
                    sourceSize: Qt.size(160, 100)
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    opacity: 0.8
                }
                Rectangle {
                    visible: screen.live
                    x: 5; y: 5
                    width: liveText.implicitWidth + 8; height: 13
                    color: "#b0000000"
                    Text { id: liveText; anchors.centerIn: parent; text: "LIVE"; color: "#ffffff"; font.family: Theme.font; font.pixelSize: 8; font.weight: Font.Bold; font.letterSpacing: 1 }
                }
                Text {
                    visible: screen.modelData.index < 10
                    anchors.right: parent.right; anchors.top: parent.top; anchors.margins: 5
                    text: screen.modelData.index === 9 ? "0" : String(screen.modelData.index + 1)
                    color: screen.chosen ? Theme.text : Theme.dim
                    font.family: Theme.font; font.pixelSize: 10; font.weight: Font.Bold
                }
                Text {
                    anchors.left: parent.left; anchors.bottom: parent.bottom; anchors.margins: 5
                    width: parent.width - 10
                    text: screen.modelData.name
                    elide: Text.ElideRight
                    color: screen.chosen ? Theme.text : Theme.muted
                    style: Text.Outline; styleColor: "#80000000"
                    font.family: Theme.font; font.pixelSize: 10; font.weight: Font.DemiBold
                }
            }
            MouseArea { anchors.fill: parent; onClicked: map.picked(screen.modelData.name) }
        }
    }
}
