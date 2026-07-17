pragma Singleton

import QtQuick

QtObject {
    property bool reduceMotion: false

    readonly property int instant: 0
    readonly property int fast: reduceMotion ? instant : 120
    readonly property int standard: reduceMotion ? instant : 200
    readonly property int gauge: reduceMotion ? instant : 400
    readonly property int drawer: reduceMotion ? instant : 260
    readonly property int pulse: reduceMotion ? instant : 300
    readonly property int sidebarCollapse: reduceMotion ? instant : 200
    readonly property int toast: reduceMotion ? instant : 220
    readonly property int sweep: reduceMotion ? instant : 24000
    readonly property int breathe: reduceMotion ? instant : 2000
    readonly property int shimmer: reduceMotion ? instant : 1200
    readonly property int stagger: reduceMotion ? instant : 40

    readonly property int fastEasing: Easing.OutQuad
    readonly property int standardEasing: Easing.OutCubic
    readonly property int gaugeEasing: Easing.OutCubic
    readonly property int drawerEasing: Easing.OutCubic
    readonly property int pulseEasing: Easing.InOutQuad
    readonly property int sidebarCollapseEasing: Easing.OutCubic
    readonly property int toastEasing: Easing.OutBack
    readonly property int sweepEasing: Easing.Linear
    readonly property int breatheEasing: Easing.InOutSine
    readonly property int shimmerEasing: Easing.Linear
    readonly property bool animationsEnabled: !reduceMotion
}
