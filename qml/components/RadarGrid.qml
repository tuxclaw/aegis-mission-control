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
        clip: true
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
}
