pragma Singleton
import QtQuick

/*
    Theme — design tokens for the Liquid Glass kit.
    Tuned for dark dashboard backdrops. Adjust freely.
*/
QtObject {
    // ---- Accent ----
    readonly property color accent:     "#00d4ff"
    readonly property color accentDeep: "#0891b2"
    readonly property color positive:   "#22e39a"
    readonly property color negative:   "#ff3366"

    // ---- Text ----
    readonly property color textPrimary:   "#e6f0ff"
    readonly property color textSecondary: "#8fa3c8"
    readonly property color textOnAccent:  "#0B1220"

    // ---- Glass material ----
    readonly property color glassTint:        "#0d1424"
    readonly property real  glassTintOpacity: 0.85
    readonly property color glassBorder:      Qt.rgba(0, 212 / 255, 1, 0.18)
    readonly property real  glassBlur:        1.0         // 0..1
    readonly property int   glassBlurMax:     48          // px kernel ceiling
    readonly property real  glassSaturation:  0.3         // subtle vibrancy boost

    // ---- Radii ----
    readonly property real radiusSmall:  10
    readonly property real radiusMedium: 16
    readonly property real radiusLarge:  22
    readonly property real radiusPill:   999

    // ---- Motion ----
    readonly property int durFast: 120
    readonly property int durMed:  220
    readonly property int durSlow: 420

    // ---- Type scale ----
    readonly property int fontSizeSmall:   12
    readonly property int fontSizeBody:    14
    readonly property int fontSizeTitle:   18
    readonly property int fontSizeDisplay: 30
}
