import QtQuick
import QtQuick.Controls.Basic

/*
    GlassTextField — text input on a glass pane with an accent focus ring.

    GlassTextField { backdrop: scene; placeholderText: "Filter services…" }
*/
TextField {
    id: root

    property Item backdrop: null

    color: Theme.textPrimary
    placeholderTextColor: Theme.textSecondary
    selectionColor: Theme.accentDeep
    selectedTextColor: "#FFFFFF"
    font.pixelSize: Theme.fontSizeBody

    leftPadding: 16
    rightPadding: 16
    topPadding: 12
    bottomPadding: 12

    background: GlassSurface {
        backdrop: root.backdrop
        radius: Theme.radiusMedium
        dropShadow: false
        tintOpacity: root.activeFocus ? 0.52 : Theme.glassTintOpacity
        Behavior on tintOpacity {
            NumberAnimation { duration: Theme.durFast }
        }

        Rectangle {
            anchors.fill: parent
            radius: Theme.radiusMedium
            color: "transparent"
            border.width: 2
            border.color: Theme.accent
            opacity: root.activeFocus ? 1 : 0
            Behavior on opacity {
                NumberAnimation { duration: Theme.durFast }
            }
        }
    }
}
