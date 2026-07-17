pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Shapes
import "../theme"

Button {
    id: root

    property bool busy: false
    property bool destructive: false
    readonly property color actionColor: destructive ? Theme.alert : Theme.accent

    implicitWidth: Math.max(Theme.buttonMinWidth, contentRow.implicitWidth + Theme.space.xl * 2)
    implicitHeight: Theme.buttonHeight
    enabled: !busy
    scale: down ? Theme.pressScale : 1

    Accessible.name: text
    Accessible.role: Accessible.Button
    Accessible.description: busy ? qsTr("Action in progress") : ""

    background: Rectangle {
        radius: Theme.radiusControl
        color: root.down && !root.destructive ? Theme.accentDim : root.actionColor
        opacity: root.enabled ? 1 : 0.45
        border.width: root.activeFocus ? Theme.focusBorderWidth : 0
        border.color: Theme.textPrimary
        layer.enabled: root.hovered && root.enabled
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: root.actionColor
            shadowBlur: 1
            blurMax: Theme.glowBlur
        }

        Behavior on color {
            ColorAnimation {
                duration: Motion.fast
                easing.type: Motion.fastEasing
            }
        }
    }

    contentItem: Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: Theme.space.sm

        Item {
            visible: root.busy
            width: Theme.spinnerSize
            height: Theme.spinnerSize

            Shape {
                anchors.fill: parent
                ShapePath {
                    fillColor: Theme.transparent
                    strokeColor: Theme.bg
                    strokeWidth: Theme.spinnerStrokeWidth
                    capStyle: ShapePath.RoundCap
                    PathAngleArc {
                        centerX: Theme.spinnerSize / 2
                        centerY: Theme.spinnerSize / 2
                        radiusX: (Theme.spinnerSize - Theme.spinnerStrokeWidth) / 2
                        radiusY: radiusX
                        startAngle: 0
                        sweepAngle: 270
                    }
                }
                RotationAnimator on rotation {
                    from: 0
                    to: 360
                    duration: Motion.shimmer
                    loops: Animation.Infinite
                    running: root.busy && Motion.animationsEnabled
                }
            }
        }

        Text {
            text: root.busy ? qsTr("Working…") : root.text
            color: Theme.bg
            font.family: Typography.label.family
            font.pixelSize: Typography.label.pixelSize
            font.weight: Font.DemiBold
            font.letterSpacing: Typography.label.letterSpacing
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: Motion.fast
            easing.type: Motion.fastEasing
        }
    }
}
