import QtQuick
import QtQuick.Controls.Basic

/*
    GlassButton — pill-shaped glass button.

    GlassButton { backdrop: scene; text: "Apply" }
    GlassButton { backdrop: scene; text: "Deploy"; prominent: true }  // accent-filled
*/
Button {
    id: root

    property Item backdrop: null
    property bool prominent: false

    horizontalPadding: 22
    verticalPadding: 12
    font.pixelSize: Theme.fontSizeBody
    font.weight: Font.DemiBold

    scale: pressed ? 0.96 : 1.0
    Behavior on scale {
        NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutQuad }
    }

    background: GlassSurface {
        backdrop: root.backdrop
        radius: Theme.radiusPill
        dropShadow: root.prominent
        shadowOpacity: 0.3
        tint: root.prominent ? Theme.accent : Theme.glassTint
        tintOpacity: root.prominent
                     ? (root.hovered ? 1.0 : 0.88)
                     : (root.hovered ? 0.55 : Theme.glassTintOpacity)
        Behavior on tintOpacity {
            NumberAnimation { duration: Theme.durFast }
        }

        // Keyboard focus ring
        Rectangle {
            anchors.fill: parent
            anchors.margins: -4
            radius: height / 2
            color: "transparent"
            border.width: 2
            border.color: Theme.accent
            visible: root.visualFocus
        }
    }

    contentItem: Text {
        text: root.text
        font: root.font
        color: root.prominent ? Theme.textOnAccent : Theme.textPrimary
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
