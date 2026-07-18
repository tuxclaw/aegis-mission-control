pragma Singleton

import QtQuick

QtObject {
    component SpacingScale: QtObject {
        readonly property int xs: 4
        readonly property int sm: 8
        readonly property int md: 12
        readonly property int lg: 16
        readonly property int xl: 24
        readonly property int xxl: 32
    }

    component RadiusScale: QtObject {
        readonly property int card: 14
        readonly property int control: 10
        readonly property int pill: 999
    }

    // Midnight Command palette.
    readonly property color bg: "#0a0e1a"
    readonly property color bgElevated: "#0d1424"
    readonly property color gridLine: Qt.rgba(0, 212 / 255, 1, 0.025)
    readonly property color panel: Qt.rgba(15 / 255, 25 / 255, 50 / 255, 0.85)
    readonly property color panelBorder: Qt.rgba(0, 212 / 255, 1, 0.18)
    readonly property color panelGlow: Qt.rgba(0, 212 / 255, 1, 0.12)
    readonly property color accent: "#00d4ff"
    readonly property color accentDim: "#0891b2"
    readonly property color accentSoft: Qt.rgba(0, 212 / 255, 1, 0.15)
    readonly property color warn: "#ffa500"
    readonly property color alert: "#ff3366"
    readonly property color ok: "#22e39a"
    readonly property color textPrimary: "#e6f0ff"
    readonly property color textSecondary: "#a8bcd8"
    readonly property color textMuted: "#6b7ea0"
    readonly property color divider: Qt.rgba(143 / 255, 163 / 255, 200 / 255, 0.12)
    readonly property color shadow: Qt.rgba(0, 0, 0, 0.55)
    readonly property color transparent: "transparent"

    // Semantic supporting colors derived from the core palette.
    readonly property color modalBackdrop: Qt.rgba(2 / 255, 5 / 255, 12 / 255, 0.72)
    readonly property color alertSoft: Qt.rgba(1, 51 / 255, 102 / 255, 0.15)
    readonly property color warnSoft: Qt.rgba(1, 165 / 255, 0, 0.15)
    readonly property color okSoft: Qt.rgba(34 / 255, 227 / 255, 154 / 255, 0.15)
    readonly property color skeletonBase: Qt.rgba(143 / 255, 163 / 255, 200 / 255, 0.08)
    readonly property color skeletonHighlight: Qt.rgba(0, 212 / 255, 1, 0.12)

    readonly property SpacingScale space: SpacingScale {}
    readonly property RadiusScale radii: RadiusScale {}

    readonly property int radiusCard: radii.card
    readonly property int radiusControl: radii.control
    readonly property int radiusPill: radii.pill
    readonly property int borderWidth: 1
    readonly property int focusBorderWidth: 2
    readonly property int activeBarWidth: 3
    readonly property int cardShadowY: 8
    readonly property int cardShadowBlur: 32
    readonly property int glowBlur: 24
    readonly property int cardPadding: 20
    readonly property int gridPitch: 32
    readonly property int gridDotSize: 2
    readonly property int enterOffset: 12
    readonly property int interactiveDuration: 160

    readonly property int sidebarExpandedWidth: 224
    readonly property int sidebarCollapsedWidth: 64
    readonly property int sidebarAutoCollapseWidth: 1180
    readonly property int sidebarItemHeight: 44
    readonly property int topBarHeight: 56
    readonly property int statusBarHeight: 28
    readonly property int iconSize: 20
    readonly property int wordmarkIconSize: 32
    readonly property int badgeMinWidth: 20
    readonly property int dividerLength: 16
    readonly property int defaultWindowWidth: 1440
    readonly property int defaultWindowHeight: 900
    readonly property int minimumWindowWidth: 1024
    readonly property int minimumWindowHeight: 720
    readonly property real wordmarkTracking: 2
    readonly property real eyebrowTracking: 1.6

    readonly property int buttonHeight: 40
    readonly property int buttonMinWidth: 104
    readonly property int compactButtonSize: 36
    readonly property int spinnerSize: 16
    readonly property int spinnerStrokeWidth: 2
    readonly property int dialogWidth: 480
    readonly property int dialogMaxWidthMargin: 64
    readonly property int emptyStateIconSize: 48
    readonly property int toastWidth: 360
    readonly property int toastMinHeight: 72
    readonly property int toastHostWidth: toastWidth
    readonly property int toastStackSpacing: 10
    readonly property int toastDuration: 4000
    readonly property int errorToastDuration: 8000

    readonly property int gaugeSize: 168
    readonly property int gaugeArcInset: 10
    readonly property int gaugeArcWidth: 8
    readonly property int gaugePulseThreshold: 5
    readonly property real gaugePulseScale: 1.04
    readonly property real pressScale: 0.98
    readonly property int ringDefaultSize: 52
    readonly property int ringStrokeWidth: 2
    readonly property int ringHighlightWidth: 3
    readonly property int skeletonHeight: 14
    readonly property int skeletonRowHeight: 56
    readonly property int defaultSkeletonCount: 4
    readonly property int minimumCardWidth: 240

    readonly property int statusBarMediumWidth: 1120
    readonly property int statusBarCompactWidth: 900
    readonly property int statusBarSegmentMinWidth: 112

    // View layout tokens
    readonly property int viewPadding: space.xl
    readonly property int viewGutter: space.lg
    readonly property int tableHeaderHeight: 40
    readonly property int tableRowHeight: 52
    readonly property int tableActionWidth: 112
    readonly property int editorDrawerWidth: 440
    readonly property int splitPaneMinimumWidth: 280
    readonly property int contentMinimumHeight: 320
    readonly property int calendarCellMinimumHeight: 92
    readonly property int agentCardHeight: 236
    readonly property int modelCardHeight: 156
    readonly property int fieldHeight: 40
    readonly property int textAreaMinimumHeight: 112
    readonly property int statusDotSize: 10
    readonly property int usageBarHeight: 10
    readonly property int avatarSize: 44
    readonly property int chipHeight: 24
    readonly property int settingsLabelWidth: 152
}
