import QtQuick
import QtQuick.Controls.Basic

/*
    GlassToggle — switch with a frosted track and a springy white knob.
*/
Switch {
    id: root

    indicator: Rectangle {
        implicitWidth: 52
        implicitHeight: 30
        x: root.leftPadding
        y: parent.height / 2 - height / 2
        radius: height / 2
        color: root.checked ? Theme.accent : Qt.rgba(1, 1, 1, 0.14)
        border.width: 1
        border.color: root.checked ? Qt.rgba(1, 1, 1, 0.35) : Qt.rgba(1, 1, 1, 0.18)
        Behavior on color {
            ColorAnimation { duration: Theme.durMed }
        }

        Rectangle {
            x: root.checked ? parent.width - width - 3 : 3
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            height: 24
            radius: 12
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#FFFFFF" }
                GradientStop { position: 1.0; color: "#E4EBF5" }
            }
            Behavior on x {
                NumberAnimation {
                    duration: Theme.durMed
                    easing.type: Easing.OutBack
                    easing.overshoot: 1.1
                }
            }
        }
    }

    contentItem: Text {
        text: root.text
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeBody
        leftPadding: root.indicator.width + 10
        verticalAlignment: Text.AlignVCenter
    }
}
