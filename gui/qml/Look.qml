pragma Singleton
import QtQuick

/// Every visual number in one place. Tweak these, no logic changes needed.
QtObject {
    // grid
    property int thumb: 180
    property int gap: 18
    property int pad: 36
    property int pageSlide: 170

    // card
    property int cardRadius: 6
    property int borderWidth: 1
    property int borderWidthSelected: 2
    property int imageInset: 2          // puts the square corners inside the rounded arc
    property bool roundPreviews: false  // mask needs its source rendered; keep off until verified
    property real hoverScale: 0.025

    // shadow / glow.  Keep blurMax under ~thumb/8 or the square turns into a circle.
    property real blurMax: 14
    property real blurMaxSelected: 18
    property real shadowBlur: 0.45
    property real shadowBlurSelected: 0.73
    property real shadowOpacity: 0.40
    property real shadowOpacitySelected: 0.62
    property real shadowOpacityHover: 0.10
    property real shadowOffset: 3
}
