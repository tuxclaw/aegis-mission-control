import QtQuick
import QtQuick.Shapes
import "../theme"

Item {
    id: root

    property string label: ""
    property real value: Number.NaN
    property string unit: "%"
    property string valueText: ""
    property string unavailableText: qsTr("n/a")
    property string subtext: ""
    property color accentColor: Theme.accent
    property bool available: true
    property real pulseThreshold: Theme.gaugePulseThreshold
    property real warnThreshold: 75
    property real alertThreshold: 90

    readonly property bool hasValue: available && !Number.isNaN(value)
    readonly property color gaugeColor: !hasValue ? Theme.textMuted : value >= alertThreshold ? Theme.alert : value >= warnThreshold ? Theme.warn : accentColor
    property real animatedValue: 0
    property real previousValue: Number.NaN
    property real pulseGlow: 0

    implicitWidth: Theme.gaugeSize
    implicitHeight: Theme.gaugeSize + labelText.implicitHeight + Theme.space.sm

    onValueChanged: updateValue()
    onAvailableChanged: updateValue()

    function updateValue() {
        if (!hasValue) {
            animatedValue = 0;
            previousValue = Number.NaN;
            return;
        }

        const bounded = Math.max(0, Math.min(100, value));
        if (!Number.isNaN(previousValue) && Math.abs(bounded - previousValue) >= pulseThreshold && Motion.animationsEnabled) {
            pulseAnimation.restart();
        }
        previousValue = bounded;
        animatedValue = bounded;
    }

    Text {
        id: labelText
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }
        text: root.label.toUpperCase()
        color: Theme.textSecondary
        font.family: Typography.label.family
        font.pixelSize: Typography.label.pixelSize
        font.weight: Typography.label.weight
        font.letterSpacing: Typography.label.letterSpacing
    }

    Item {
        id: gaugeBody
        width: Theme.gaugeSize
        height: Theme.gaugeSize
        anchors {
            top: labelText.bottom
            topMargin: Theme.space.sm
            horizontalCenter: parent.horizontalCenter
        }

        Rectangle {
            anchors.centerIn: parent
            width: parent.width - Theme.gaugeArcInset
            height: width
            radius: width / 2
            color: Theme.transparent
            border.width: Theme.gaugeArcWidth
            border.color: root.gaugeColor
            opacity: root.pulseGlow
            scale: Theme.gaugePulseScale
        }

        Shape {
            anchors.fill: parent
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                fillColor: Theme.transparent
                strokeColor: root.hasValue ? Theme.divider : Theme.textMuted
                strokeWidth: Theme.gaugeArcWidth
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: gaugeBody.width / 2
                    centerY: gaugeBody.height / 2
                    radiusX: (gaugeBody.width - Theme.gaugeArcInset * 2) / 2
                    radiusY: radiusX
                    startAngle: 135
                    sweepAngle: 270
                }
            }

            ShapePath {
                fillColor: Theme.transparent
                strokeColor: root.gaugeColor
                strokeWidth: Theme.gaugeArcWidth
                capStyle: ShapePath.RoundCap
                PathAngleArc {
                    centerX: gaugeBody.width / 2
                    centerY: gaugeBody.height / 2
                    radiusX: (gaugeBody.width - Theme.gaugeArcInset * 2) / 2
                    radiusY: radiusX
                    startAngle: 135
                    sweepAngle: root.hasValue ? root.animatedValue * 2.7 : 0
                }
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: Theme.space.xs

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.valueText.length > 0 ? root.valueText : root.hasValue ? Math.round(root.animatedValue) + root.unit : root.unavailableText
                color: root.hasValue ? Theme.textPrimary : Theme.textMuted
                font.family: Typography.display.family
                font.pixelSize: Typography.display.pixelSize
                font.weight: Typography.display.weight
                font.letterSpacing: Typography.display.letterSpacing
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.subtext
                visible: text.length > 0
                color: Theme.textMuted
                font.family: Typography.dataSmall.family
                font.pixelSize: Typography.dataSmall.pixelSize
                font.weight: Typography.dataSmall.weight
                font.letterSpacing: Typography.dataSmall.letterSpacing
            }
        }
    }

    Behavior on animatedValue {
        NumberAnimation {
            duration: Motion.gauge
            easing.type: Motion.gaugeEasing
        }
    }

    SequentialAnimation {
        id: pulseAnimation
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "scale"
                to: Theme.gaugePulseScale
                duration: Motion.pulse / 2
                easing.type: Motion.pulseEasing
            }
            NumberAnimation {
                target: root
                property: "pulseGlow"
                to: 0.8
                duration: Motion.pulse / 2
                easing.type: Motion.pulseEasing
            }
        }
        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "scale"
                to: 1
                duration: Motion.pulse / 2
                easing.type: Motion.pulseEasing
            }
            NumberAnimation {
                target: root
                property: "pulseGlow"
                to: 0
                duration: Motion.pulse / 2
                easing.type: Motion.pulseEasing
            }
        }
    }

    Component.onCompleted: updateValue()
}
