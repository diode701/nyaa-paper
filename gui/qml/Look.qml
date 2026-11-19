pragma Singleton
import QtQuick

/// Geometry copied from WallpaperBrowser: THUMB_SIZE 180, HGAP/VGAP 15,
/// grid padding 20. Card chrome copied from dark-theme.css.
QtObject {
    // grid
    property int thumb: 180
    property int gap: 15
    property int pad: 20
    property int pageSlide: 170

    // card
    property int cardRadius: 8
    property int borderWidth: 1
    property int borderWidthSelected: 3
    property int imageInset: 5          // 170 px image inside the 180 px cell
    property bool roundPreviews: false
    property real hoverScale: 0.02

    // shadow: dropshadow(gaussian, rgba(0,0,0,0.5), 10, 0, 0, 2)
    property real blurMax: 10
    property real blurMaxSelected: 15
    property real shadowBlur: 0.6
    property real shadowBlurSelected: 0.85
    property real shadowOpacity: 0.50
    property real shadowOpacitySelected: 0.65
    property real shadowOpacityHover: 0.10
    property real shadowOffset: 2
}
