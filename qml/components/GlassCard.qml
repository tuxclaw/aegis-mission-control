pragma ComponentBehavior: Bound

import QtQuick
import LiquidGlass as LG
import "../theme"

LG.GlassCard {
    id: root

    default property alias contentItem: contentColumn.data
    property int padding: Theme.cardPadding
    property bool interactive: false
    property int enterDelay: 0
    readonly property bool hovered: interactive && hoverHandler.hovered

    contentPadding: padding
    radius: Theme.radiusCard
    tintOpacity: 0.85
    brightness: hovered ? 0.08 : 0
    implicitWidth: Theme.minimumCardWidth
    implicitHeight: padding * 2 + (title.length > 0 ? LG.Theme.fontSizeSmall + Theme.space.md : 0) + contentColumn.implicitHeight
    opacity: 0

    transform: Translate {
        id: enterTranslation
        y: Theme.enterOffset
    }

    Column {
        id: contentColumn
        anchors.fill: parent
        spacing: Theme.space.md
    }

    HoverHandler {
        id: hoverHandler
        enabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
    }

    Behavior on brightness {
        NumberAnimation {
            duration: Motion.reduceMotion ? Motion.instant : Theme.interactiveDuration
            easing.type: Motion.fastEasing
        }
    }

    ParallelAnimation {
        id: enterAnimation
        NumberAnimation {
            target: root
            property: "opacity"
            from: 0
            to: 1
            duration: Motion.standard
            easing.type: Motion.standardEasing
        }
        NumberAnimation {
            target: enterTranslation
            property: "y"
            from: Theme.enterOffset
            to: 0
            duration: Motion.standard
            easing.type: Motion.standardEasing
        }
    }

    Timer {
        id: enterTimer
        interval: root.enterDelay
        onTriggered: enterAnimation.start()
    }

    Component.onCompleted: {
        if (enterDelay > 0)
            enterTimer.start();
        else
            enterAnimation.start();
    }
}
