import QtQuick
import "../theme"

Item {
    id: root

    Accessible.ignored: true

    Rectangle {
        anchors.fill: parent
        color: Theme.bg
    }

    Canvas {
        id: gridCanvas
        anchors.fill: parent
        renderStrategy: Canvas.Cooperative

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const context = getContext("2d");
            context.reset();
            context.fillStyle = Theme.gridLine;

            for (let x = Theme.gridPitch / 2; x < width; x += Theme.gridPitch) {
                for (let y = Theme.gridPitch / 2; y < height; y += Theme.gridPitch) {
                    context.beginPath();
                    context.arc(x, y, Theme.gridDotSize / 2, 0, Math.PI * 2);
                    context.fill();
                }
            }
        }
    }

    Canvas {
        id: sweepCanvas
        width: Math.max(root.width, root.height) * 2
        height: width
        anchors.centerIn: parent
        opacity: Motion.reduceMotion ? 0 : 1
        transformOrigin: Item.Center
        renderStrategy: Canvas.Cooperative

        onWidthChanged: requestPaint()
        onHeightChanged: requestPaint()
        onPaint: {
            const context = getContext("2d");
            const center = width / 2;
            const radius = width / 2;
            const slices = 18;
            const sweepRadians = Math.PI / 3;
            context.reset();

            for (let slice = 0; slice < slices; ++slice) {
                const start = -sweepRadians * slice / slices;
                const end = -sweepRadians * (slice + 1) / slices;
                const alpha = Theme.sweepOpacity * (1 - slice / slices);
                context.globalAlpha = alpha;
                context.fillStyle = Theme.accent;
                context.beginPath();
                context.moveTo(center, center);
                context.arc(center, center, radius, end, start);
                context.closePath();
                context.fill();
            }
            context.globalAlpha = 1;
        }

        RotationAnimator on rotation {
            from: 0
            to: 360
            duration: Motion.sweep
            easing.type: Motion.sweepEasing
            loops: Animation.Infinite
            running: Motion.animationsEnabled && root.visible
        }
    }
}
