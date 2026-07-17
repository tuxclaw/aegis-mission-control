import QtQuick
import QtQuick.Controls
import "../theme"

Button {
    id: root

    property bool busy: false
    property bool destructive: false
    readonly property color actionColor: destructive ? Theme.alert : Theme.textSecondary

    implicitWidth: Math.max(Theme.buttonMinWidth, labelText.implicitWidth + Theme.space.lg * 2)
    implicitHeight: Theme.buttonHeight
    enabled: !busy
    scale: down ? Theme.pressScale : 1

    Accessible.name: text
    Accessible.role: Accessible.Button

    background: Rectangle {
        radius: Theme.radiusControl
        color: root.hovered ? Theme.accentSoft : Theme.transparent
        border.width: root.activeFocus ? Theme.focusBorderWidth : 0
        border.color: Theme.accent

        Behavior on color {
            ColorAnimation {
                duration: Motion.fast
                easing.type: Motion.fastEasing
            }
        }
    }

    contentItem: Text {
        id: labelText
        text: root.busy ? qsTr("Working…") : root.text
        color: root.down ? Theme.accentDim : root.hovered ? Theme.textPrimary : root.actionColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: Typography.label.family
        font.pixelSize: Typography.label.pixelSize
        font.weight: Typography.label.weight
        font.letterSpacing: Typography.label.letterSpacing

        Behavior on color {
            ColorAnimation {
                duration: Motion.fast
                easing.type: Motion.fastEasing
            }
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: Motion.fast
            easing.type: Motion.fastEasing
        }
    }
}
