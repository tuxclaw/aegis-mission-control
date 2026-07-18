import QtQuick
import QtQuick.Controls.Basic

/*
    GlassSlider — frosted track, accent gradient fill, glossy handle.
*/
Slider {
    id: root

    implicitWidth: 220
    implicitHeight: 30

    background: Rectangle {
        x: root.leftPadding
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: root.availableWidth
        height: 6
        radius: 3
        color: Qt.rgba(1, 1, 1, 0.14)
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.10)

        Rectangle {
            width: root.visualPosition * parent.width
            height: parent.height
            radius: 3
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accentDeep }
                GradientStop { position: 1.0; color: Theme.accent }
            }
        }
    }

    handle: Rectangle {
        x: root.leftPadding + root.visualPosition * (root.availableWidth - width)
        y: root.topPadding + root.availableHeight / 2 - height / 2
        width: 22
        height: 22
        radius: 11
        border.width: 1
        border.color: Qt.rgba(0, 0, 0, 0.18)
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#FFFFFF" }
            GradientStop { position: 1.0; color: "#DCE6F2" }
        }

        scale: root.pressed ? 1.15 : 1.0
        Behavior on scale {
            NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad }
        }
    }
}
