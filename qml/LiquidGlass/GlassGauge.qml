import QtQuick

/*
    GlassGauge — 270° radial gauge for utilization/temperature readouts.
    Color shifts accent → amber → red as the value climbs.

    GlassGauge {
        value: stats.cpuUsage          // 0..1
        label: "CPU"
        subText: "54°C"
    }
*/
Item {
    id: root

    property real value: 0                       // 0..1
    property string label: ""
    property string valueText: Math.round(Math.min(1, Math.max(0, value)) * 100) + "%"
    property string subText: ""
    property real thickness: 10
    property color trackColor: Qt.rgba(1, 1, 1, 0.12)
    property color color: value < 0.7 ? Theme.accent
                        : value < 0.9 ? "#FFC864"
                        : Theme.negative

    implicitWidth: 150
    implicitHeight: 150

    Behavior on value {
        NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic }
    }

    onValueChanged: canvas.requestPaint()
    onColorChanged: canvas.requestPaint()

    Canvas {
        id: canvas
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()
            const cx = width / 2
            const cy = height / 2
            const r = Math.min(width, height) / 2 - root.thickness / 2 - 2
            const start = Math.PI * 0.75
            const sweep = Math.PI * 1.5

            ctx.lineWidth = root.thickness
            ctx.lineCap = "round"

            ctx.strokeStyle = root.trackColor
            ctx.beginPath()
            ctx.arc(cx, cy, r, start, start + sweep)
            ctx.stroke()

            const v = Math.min(1, Math.max(0, root.value))
            if (v > 0.004) {
                ctx.strokeStyle = root.color
                ctx.beginPath()
                ctx.arc(cx, cy, r, start, start + sweep * v)
                ctx.stroke()
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 2
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: root.valueText
            color: Theme.textPrimary
            font.pixelSize: 26
            font.weight: Font.DemiBold
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.subText !== ""
            text: root.subText
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSmall
        }
    }

    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        visible: root.label !== ""
        text: root.label
        color: Theme.textSecondary
        font.pixelSize: Theme.fontSizeSmall
        font.letterSpacing: 1.2
        font.capitalization: Font.AllUppercase
    }
}
