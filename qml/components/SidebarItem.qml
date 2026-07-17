import QtQuick
import QtQuick.Controls
import "../theme"

ToolButton {
    id: root

    property url iconSource
    property string label: ""
    property bool active: false
    property string badgeText: ""
    property color badgeColor: Theme.accent
    property bool showLabel: true
    readonly property color foregroundColor: down ? Theme.accentDim : active ? Theme.accent : hovered ? Theme.textPrimary : Theme.textSecondary

    implicitWidth: Theme.sidebarExpandedWidth - Theme.space.lg * 2
    implicitHeight: Theme.sidebarItemHeight
    text: label
    display: showLabel ? AbstractButton.TextBesideIcon : AbstractButton.IconOnly
    spacing: Theme.space.md
    icon.source: iconSource
    icon.width: Theme.iconSize
    icon.height: Theme.iconSize
    icon.color: down ? Theme.accentDim : active || hovered ? Theme.accent : Theme.textSecondary
    palette.buttonText: foregroundColor
    font.family: Typography.label.family
    font.pixelSize: Typography.label.pixelSize
    font.weight: Typography.label.weight
    font.letterSpacing: Typography.label.letterSpacing
    scale: down ? Theme.pressScale : 1

    Accessible.name: label
    Accessible.role: Accessible.Button

    background: Rectangle {
        radius: Theme.radiusControl
        color: root.active || root.hovered ? Theme.accentSoft : Theme.transparent
        border.width: root.activeFocus ? Theme.focusBorderWidth : 0
        border.color: Theme.accent

        Rectangle {
            width: Theme.activeBarWidth
            height: root.active ? parent.height - Theme.space.md : 0
            anchors {
                left: parent.left
                verticalCenter: parent.verticalCenter
            }
            radius: Theme.radiusPill
            color: root.down ? Theme.accentDim : Theme.accent

            Behavior on height {
                NumberAnimation {
                    duration: Motion.fast
                    easing.type: Motion.fastEasing
                }
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: Motion.fast
                easing.type: Motion.fastEasing
            }
        }
    }

    Rectangle {
        visible: root.badgeText.length > 0
        anchors {
            right: parent.right
            rightMargin: Theme.space.sm
            verticalCenter: parent.verticalCenter
        }
        implicitWidth: Math.max(Theme.badgeMinWidth, badgeLabel.implicitWidth + Theme.space.sm)
        implicitHeight: Theme.badgeMinWidth
        radius: Theme.radiusPill
        color: root.badgeColor

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.badgeText
            color: Theme.bg
            font.family: Typography.caption.family
            font.pixelSize: Typography.caption.pixelSize
            font.weight: Font.Bold
            font.letterSpacing: Typography.caption.letterSpacing
        }
    }

    ToolTip.visible: hovered && !showLabel
    ToolTip.text: label
    ToolTip.delay: Motion.fast

    Behavior on scale {
        NumberAnimation {
            duration: Motion.fast
            easing.type: Motion.fastEasing
        }
    }
}
