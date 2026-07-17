pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Item {
    id: root

    property int skeletonCount: Theme.defaultSkeletonCount
    property int rowHeight: Theme.skeletonRowHeight

    implicitWidth: Theme.minimumCardWidth
    implicitHeight: skeletonCount * rowHeight + Math.max(0, skeletonCount - 1) * Theme.space.md
    clip: true
    Accessible.name: qsTr("Loading")
    Accessible.role: Accessible.Indicator

    Column {
        anchors.fill: parent
        spacing: Theme.space.md

        Repeater {
            model: root.skeletonCount

            Rectangle {
                required property int index
                width: root.width * (index % 3 === 0 ? 1 : index % 3 === 1 ? 0.82 : 0.68)
                height: root.rowHeight
                radius: Theme.radiusControl
                color: Theme.skeletonBase
            }
        }
    }

    Rectangle {
        id: shimmerBand
        width: root.width / 3
        height: root.height
        x: -width
        visible: Motion.animationsEnabled
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0
                color: Theme.transparent
            }
            GradientStop {
                position: 0.5
                color: Theme.skeletonHighlight
            }
            GradientStop {
                position: 1
                color: Theme.transparent
            }
        }

        NumberAnimation on x {
            from: -shimmerBand.width
            to: root.width
            duration: Motion.shimmer
            easing.type: Motion.shimmerEasing
            loops: Animation.Infinite
            running: Motion.animationsEnabled && root.visible
        }
    }
}
