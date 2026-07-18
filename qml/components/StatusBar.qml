import QtQuick
import QtQuick.Layouts
import LiquidGlass as LG
import "../theme"

LG.GlassSurface {
    id: root

    property string connectionState: "unknown"
    property string connectionLabel: qsTr("Unavailable")
    property int activeAgents: 0
    property int totalAgents: 0
    property real cpuPercent: Number.NaN
    property real memoryPercent: Number.NaN
    property string lastSync: qsTr("Never")
    property int syncSerial: 0

    readonly property color connectionColor: connectionState === "connected" ? Theme.ok : connectionState === "connecting" ? Theme.warn : connectionState === "disconnected" ? Theme.alert : Theme.textMuted

    implicitHeight: Theme.statusBarHeight
    radius: 0
    dropShadow: false
    sheen: false
    tint: Theme.bgElevated
    tintOpacity: 0.92
    bordered: true

    onSyncSerialChanged: syncPulse.restart()

    RowLayout {
        anchors {
            fill: parent
            leftMargin: Theme.space.lg
            rightMargin: Theme.space.lg
        }
        spacing: Theme.space.md

        RowLayout {
            Layout.minimumWidth: Theme.statusBarSegmentMinWidth
            Layout.fillWidth: true
            spacing: Theme.space.sm

            Rectangle {
                Layout.preferredWidth: Theme.space.sm
                Layout.preferredHeight: Theme.space.sm
                radius: width / 2
                color: root.connectionColor

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    running: root.connectionState === "connecting" && Motion.animationsEnabled && root.visible
                    NumberAnimation {
                        to: 0.3
                        duration: Motion.breathe / 2
                        easing.type: Motion.breatheEasing
                    }
                    NumberAnimation {
                        to: 1
                        duration: Motion.breathe / 2
                        easing.type: Motion.breatheEasing
                    }
                }
            }
            Text {
                Layout.fillWidth: true
                text: root.connectionLabel
                color: Theme.textSecondary
                elide: Text.ElideRight
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
                font.letterSpacing: Typography.caption.letterSpacing
            }
        }

        Rectangle {
            Layout.preferredWidth: Theme.borderWidth
            Layout.preferredHeight: Theme.dividerLength
            color: Theme.divider
        }

        Text {
            visible: root.width >= Theme.statusBarCompactWidth
            Layout.minimumWidth: visible ? Theme.statusBarSegmentMinWidth : 0
            Layout.fillWidth: visible
            text: root.width >= Theme.statusBarMediumWidth ? qsTr("Agents: %1 active / %2").arg(root.activeAgents).arg(root.totalAgents) : qsTr("%1 / %2").arg(root.activeAgents).arg(root.totalAgents)
            color: Theme.textSecondary
            elide: Text.ElideRight
            font.family: Typography.caption.family
            font.pixelSize: Typography.caption.pixelSize
            font.weight: Typography.caption.weight
            font.letterSpacing: Typography.caption.letterSpacing
        }

        Rectangle {
            visible: root.width >= Theme.statusBarCompactWidth
            Layout.preferredWidth: visible ? Theme.borderWidth : 0
            Layout.preferredHeight: Theme.dividerLength
            color: Theme.divider
        }

        Text {
            visible: root.width >= Theme.statusBarMediumWidth && !Number.isNaN(root.cpuPercent) && !Number.isNaN(root.memoryPercent)
            Layout.minimumWidth: visible ? Theme.statusBarSegmentMinWidth : 0
            text: qsTr("CPU %1%  MEM %2%").arg(Math.round(root.cpuPercent)).arg(Math.round(root.memoryPercent))
            color: Theme.textSecondary
            font.family: Typography.dataSmall.family
            font.pixelSize: Typography.dataSmall.pixelSize
            font.weight: Typography.dataSmall.weight
            font.letterSpacing: Typography.dataSmall.letterSpacing
        }

        Rectangle {
            visible: root.width >= Theme.statusBarMediumWidth
            Layout.preferredWidth: visible ? Theme.borderWidth : 0
            Layout.preferredHeight: Theme.dividerLength
            color: Theme.divider
        }

        RowLayout {
            Layout.minimumWidth: Theme.statusBarSegmentMinWidth
            Layout.fillWidth: true
            spacing: Theme.space.sm

            Text {
                id: refreshGlyph
                text: "↻"
                color: Theme.accent
                font.family: Typography.dataSmall.family
                font.pixelSize: Typography.dataSmall.pixelSize
                font.weight: Typography.dataSmall.weight
                transformOrigin: Item.Center
            }
            Text {
                Layout.fillWidth: true
                text: qsTr("Last sync %1").arg(root.lastSync)
                color: Theme.textMuted
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
                font.letterSpacing: Typography.caption.letterSpacing
            }
        }
    }

    SequentialAnimation {
        id: syncPulse
        NumberAnimation {
            target: refreshGlyph
            property: "scale"
            to: Theme.gaugePulseScale
            duration: Motion.pulse / 2
            easing.type: Motion.pulseEasing
        }
        NumberAnimation {
            target: refreshGlyph
            property: "scale"
            to: 1
            duration: Motion.pulse / 2
            easing.type: Motion.pulseEasing
        }
    }
}
