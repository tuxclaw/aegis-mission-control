import QtQuick

/*
    GlassMeter — labelled horizontal bar for per-core load, RAM, disk fill.
    Color shifts accent → amber → red as the value climbs.

    GlassMeter { label: "Core 3"; value: 0.62 }
*/
Item {
    id: root

    property real value: 0                       // 0..1
    property string label: ""
    property string valueText: Math.round(Math.min(1, Math.max(0, value)) * 100) + "%"
    property color barColor: value < 0.7 ? Theme.accent
                           : value < 0.9 ? "#FFC864"
                           : Theme.negative

    implicitWidth: 160
    implicitHeight: label !== "" ? 32 : 10

    Behavior on value {
        NumberAnimation { duration: Theme.durMed; easing.type: Easing.OutQuad }
    }

    Text {
        visible: root.label !== ""
        text: root.label
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
    }

    Text {
        visible: root.label !== ""
        anchors.right: parent.right
        text: root.valueText
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeSmall
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 6
        radius: 3
        color: Qt.rgba(1, 1, 1, 0.12)

        Rectangle {
            width: Math.max(height, parent.width * Math.min(1, Math.max(0, root.value)))
            height: parent.height
            radius: 3
            color: root.barColor
            Behavior on color {
                ColorAnimation { duration: Theme.durMed }
            }
        }
    }
}
