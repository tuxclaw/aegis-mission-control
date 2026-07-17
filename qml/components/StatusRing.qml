import QtQuick
import QtQuick.Effects
import QtQuick.Shapes
import "../theme"

Item {
    id: root

    enum AgentStatus {
        Active,
        Idle,
        Error,
        Unknown
    }

    default property alias content: avatarHost.data
    property int status: StatusRing.Unknown
    property int size: Theme.ringDefaultSize
    property bool pulse: true
    readonly property color statusColor: status === StatusRing.Active ? Theme.accent : status === StatusRing.Idle ? Theme.warn : status === StatusRing.Error ? Theme.alert : Theme.textMuted
    property real errorFlashOpacity: 0
    property real breathingOpacity: 0.22

    implicitWidth: size
    implicitHeight: size

    Item {
        id: avatarHost
        anchors {
            fill: parent
            margins: Theme.space.xs
        }
    }

    Rectangle {
        id: breathingGlow
        anchors.fill: parent
        radius: width / 2
        color: Theme.transparent
        border.width: Theme.ringHighlightWidth
        border.color: root.statusColor
        opacity: root.status === StatusRing.Active && root.pulse ? root.breathingOpacity : 0
        layer.enabled: root.status === StatusRing.Active && root.pulse
        layer.effect: MultiEffect {
            blurEnabled: true
            blur: 1
            blurMax: Theme.glowBlur
        }

        SequentialAnimation {
            loops: Animation.Infinite
            running: root.status === StatusRing.Active && root.pulse && Motion.animationsEnabled && root.visible
            NumberAnimation {
                target: root
                property: "breathingOpacity"
                to: 0.68
                duration: Motion.breathe / 2
                easing.type: Motion.breatheEasing
            }
            NumberAnimation {
                target: root
                property: "breathingOpacity"
                to: 0.22
                duration: Motion.breathe / 2
                easing.type: Motion.breatheEasing
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.transparent
        border.width: Theme.ringStrokeWidth
        border.color: root.statusColor
    }

    Shape {
        id: dashedHighlight
        anchors.fill: parent
        visible: root.status === StatusRing.Active
        opacity: Motion.reduceMotion ? 0.55 : 1
        transformOrigin: Item.Center

        ShapePath {
            fillColor: Theme.transparent
            strokeColor: Theme.textPrimary
            strokeWidth: Theme.ringHighlightWidth
            strokeStyle: ShapePath.DashLine
            dashPattern: [1, 5]
            capStyle: ShapePath.RoundCap
            PathAngleArc {
                centerX: dashedHighlight.width / 2
                centerY: dashedHighlight.height / 2
                radiusX: (dashedHighlight.width - Theme.ringHighlightWidth) / 2
                radiusY: radiusX
                startAngle: 0
                sweepAngle: 360
            }
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: Motion.sweep
            loops: Animation.Infinite
            easing.type: Motion.sweepEasing
            running: root.status === StatusRing.Active && root.pulse && Motion.animationsEnabled && root.visible
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: Theme.transparent
        border.width: Theme.ringHighlightWidth
        border.color: Theme.alert
        opacity: root.errorFlashOpacity
    }

    SequentialAnimation {
        id: errorFlash
        loops: 2
        NumberAnimation {
            target: root
            property: "errorFlashOpacity"
            from: 0
            to: 1
            duration: Motion.fast
            easing.type: Motion.fastEasing
        }
        NumberAnimation {
            target: root
            property: "errorFlashOpacity"
            to: 0
            duration: Motion.fast
            easing.type: Motion.fastEasing
        }
    }

    onStatusChanged: {
        if (status === StatusRing.Error && Motion.animationsEnabled)
            errorFlash.restart();
    }
}
