pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: root

    default property alias contentItem: contentColumn.data
    property string title: ""
    property int padding: Theme.cardPadding
    property bool interactive: false
    readonly property bool hovered: interactive && hoverHandler.hovered

    implicitWidth: Theme.minimumCardWidth
    implicitHeight: padding * 2 + (header.visible ? header.implicitHeight + Theme.space.md : 0) + contentColumn.implicitHeight
    opacity: 0

    transform: Translate {
        id: enterTranslation
        y: Theme.enterOffset
    }

    Rectangle {
        id: glowSurface
        anchors.fill: parent
        color: Theme.transparent
        radius: Theme.radiusCard
        border.width: Theme.borderWidth
        border.color: Theme.panelGlow
        opacity: root.hovered ? 1 : 0.55
        layer.enabled: true
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1
            blurMax: Theme.glowBlur
        }

        Behavior on opacity {
            NumberAnimation {
                duration: Motion.reduceMotion ? Motion.instant : Theme.interactiveDuration
                easing.type: Motion.fastEasing
            }
        }
    }

    Rectangle {
        id: surface
        anchors.fill: parent
        color: Theme.panel
        radius: Theme.radiusCard
        border.width: Theme.borderWidth
        border.color: root.hovered ? Theme.accent : Theme.panelBorder
        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: Theme.shadow
            shadowBlur: 1
            blurMax: Theme.cardShadowBlur
            shadowVerticalOffset: Theme.cardShadowY
        }

        Behavior on border.color {
            ColorAnimation {
                duration: Motion.reduceMotion ? Motion.instant : Theme.interactiveDuration
                easing.type: Motion.fastEasing
            }
        }
    }

    Column {
        id: contentColumn
        anchors {
            fill: parent
            margins: root.padding
        }
        spacing: Theme.space.md

        Text {
            id: header
            width: parent.width
            visible: root.title.length > 0
            text: root.title
            color: Theme.textPrimary
            font.family: Typography.heading.family
            font.pixelSize: Typography.heading.pixelSize
            font.weight: Typography.heading.weight
            font.letterSpacing: Typography.heading.letterSpacing
            elide: Text.ElideRight
        }
    }

    HoverHandler {
        id: hoverHandler
        enabled: root.interactive
        cursorShape: root.interactive ? Qt.PointingHandCursor : Qt.ArrowCursor
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

    Component.onCompleted: enterAnimation.start()
}
