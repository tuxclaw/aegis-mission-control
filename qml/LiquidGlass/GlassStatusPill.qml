import QtQuick

/*
    GlassStatusPill — status chip with a colored dot, keyed off common state
    words (running/up/healthy → green, idle → accent, paused/created → amber,
    exited/error → red). Override `pillColor` to pin a color.

    GlassStatusPill { status: "running" }
*/
Rectangle {
    id: root

    property string status: "unknown"
    property color pillColor: {
        const s = status.toLowerCase()
        if (/(run|up|healthy|active|online|ok|ready)/.test(s)) return Theme.positive
        if (/(idle|sleep)/.test(s)) return Theme.accent
        if (/(pause|creat|wait|pend|start|init)/.test(s)) return "#FFC864"
        if (/(exit|stop|dead|error|fail|off|crash)/.test(s)) return Theme.negative
        return Qt.rgba(1, 1, 1, 0.5)
    }

    implicitHeight: 24
    implicitWidth: label.implicitWidth + dot.width + 24
    radius: height / 2
    color: Qt.rgba(pillColor.r, pillColor.g, pillColor.b, 0.14)
    border.width: 1
    border.color: Qt.rgba(pillColor.r, pillColor.g, pillColor.b, 0.4)

    Rectangle {
        id: dot
        x: 9
        anchors.verticalCenter: parent.verticalCenter
        width: 7
        height: 7
        radius: 3.5
        color: root.pillColor
    }

    Text {
        id: label
        anchors.left: dot.right
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        text: root.status
        color: Theme.textPrimary
        font.pixelSize: Theme.fontSizeSmall
    }
}
