import QtQuick

/*
    GlassGraph — scrolling line graph with up to two series (e.g. net rx/tx).
    Newest samples enter from the right. Push data from your sampling signal:

        GlassGraph { id: netGraph }
        Connections {
            target: stats
            function onSampled() { netGraph.push(stats.netRx, stats.netTx) }
        }
*/
Item {
    id: root

    property int capacity: 60                    // samples kept
    property color series1Color: Theme.accent
    property color series2Color: "#B08CFF"
    property bool autoScale: true
    property real maxValue: 1                    // used when autoScale is false

    property var _s1: []
    property var _s2: []

    function push(v1, v2) {
        _s1.push(Number(v1) || 0)
        if (_s1.length > capacity) _s1.shift()
        if (v2 !== undefined) {
            _s2.push(Number(v2) || 0)
            if (_s2.length > capacity) _s2.shift()
        }
        canvas.requestPaint()
    }

    function clear() {
        _s1 = []
        _s2 = []
        canvas.requestPaint()
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const all = root._s1.concat(root._s2)
            const hi = root.autoScale
                       ? Math.max(1e-9, Math.max.apply(null, all.length ? all : [1]))
                       : Math.max(1e-9, root.maxValue)

            function draw(s, col, fill) {
                if (s.length < 2)
                    return
                const step = width / (root.capacity - 1)
                const px = i => width - (s.length - 1 - i) * step
                const py = v => height - 3 - (Math.min(v, hi) / hi) * (height - 8)

                ctx.beginPath()
                ctx.moveTo(px(0), py(s[0]))
                for (let i = 1; i < s.length; ++i)
                    ctx.lineTo(px(i), py(s[i]))
                ctx.strokeStyle = col
                ctx.lineWidth = 2
                ctx.lineJoin = "round"
                ctx.stroke()

                if (fill) {
                    ctx.lineTo(px(s.length - 1), height)
                    ctx.lineTo(px(0), height)
                    ctx.closePath()
                    ctx.fillStyle = Qt.rgba(col.r, col.g, col.b, 0.12)
                    ctx.fill()
                }
            }

            draw(root._s1, root.series1Color, true)
            draw(root._s2, root.series2Color, false)
        }
    }
}
