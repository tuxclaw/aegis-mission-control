import QtQuick
import QtQuick.Effects

/*
    GlassSurface — the core "liquid glass" material.

    Usage:
        GlassSurface {
            backdrop: scene          // the item rendered behind this surface
            width: 300; height: 160
        }

    Rules:
      - `backdrop` must NOT be an ancestor of this item (no recursion).
        Structure your scene as two siblings: a background item, and a UI
        layer on top containing the glass surfaces.
      - The backdrop capture follows this item's own x/y/width/height.
        If an *ancestor* of the surface moves (e.g. it sits inside a
        Flickable or an animated container), set `liveTracking: true`
        to re-sync the capture every frame while it moves.
*/
Item {
    id: root

    // The item captured and blurred behind the glass.
    property Item backdrop: null

    property real radius: Theme.radiusLarge
    property real blurAmount: Theme.glassBlur       // 0..1
    property int  blurMax: Theme.glassBlurMax       // blur kernel ceiling, px
    property color tint: Theme.glassTint
    property real tintOpacity: Theme.glassTintOpacity
    property real saturation: Theme.glassSaturation // -1..1
    property real brightness: 0.0                   // -1..1
    property bool bordered: true
    property bool sheen: true
    property bool dropShadow: true
    property real shadowOpacity: 0.4

    // Re-sync the backdrop capture every frame (for scrolling/moving parents).
    property bool liveTracking: false

    default property alias contentData: contentItem.data

    function _captureRect() {
        if (!backdrop)
            return Qt.rect(0, 0, 1, 1)
        const p = mapToItem(backdrop, 0, 0)
        return Qt.rect(p.x, p.y, width, height)
    }

    // ---- Drop shadow (soft, behind the pane) ----
    Rectangle {
        id: shadowSource
        width: root.width
        height: root.height
        radius: root.radius
        color: "#000000"
        visible: false
    }
    MultiEffect {
        source: shadowSource
        width: root.width
        height: root.height
        y: 10
        blurEnabled: true
        blur: 1.0
        blurMax: 32
        visible: root.dropShadow
        opacity: root.shadowOpacity
    }

    // ---- Backdrop capture ----
    ShaderEffectSource {
        id: capture
        visible: false
        live: true
        sourceItem: root.backdrop
        sourceRect: {
            // Reference these so the rect re-evaluates when the surface moves/resizes.
            root.x; root.y; root.width; root.height;
            return root._captureRect()
        }
    }

    FrameAnimation {
        running: root.liveTracking && root.backdrop !== null
        onTriggered: capture.sourceRect = root._captureRect()
    }

    // ---- Blur, vibrancy, rounded mask ----
    MultiEffect {
        anchors.fill: parent
        source: capture
        visible: root.backdrop !== null
        blurEnabled: true
        blur: root.blurAmount
        blurMax: root.blurMax
        saturation: root.saturation
        brightness: root.brightness
        maskEnabled: true
        maskSource: ShaderEffectSource {
            sourceItem: Rectangle {
                width: root.width
                height: root.height
                radius: root.radius
                color: "#ffffff"
            }
        }
    }

    // ---- Tint ----
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        color: root.tint
        opacity: root.tintOpacity
    }

    // ---- Sheen (specular light from above) ----
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        visible: root.sheen
        gradient: Gradient {
            GradientStop { position: 0.0;  color: Qt.rgba(1, 1, 1, 0.20) }
            GradientStop { position: 0.30; color: Qt.rgba(1, 1, 1, 0.05) }
            GradientStop { position: 1.0;  color: Qt.rgba(1, 1, 1, 0.00) }
        }
    }

    // ---- Hairline border ----
    Rectangle {
        anchors.fill: parent
        radius: root.radius
        visible: root.bordered
        color: "transparent"
        border.width: 1
        border.color: Theme.glassBorder
    }

    Item {
        id: contentItem
        anchors.fill: parent
    }
}
