import QtQuick

/*
    GlassCard — a GlassSurface with padding and an optional uppercase title.

    GlassCard {
        backdrop: scene
        title: "P95 latency"
        Text { text: "182 ms" }     // children land in the body area
    }
*/
GlassSurface {
    id: root

    property string title: ""
    property real contentPadding: 20
    default property alias body: bodyItem.data

    Item {
        anchors.fill: parent
        anchors.margins: root.contentPadding

        Text {
            id: titleText
            width: parent.width
            text: root.title
            visible: root.title !== ""
            height: visible ? implicitHeight : 0
            color: Theme.textSecondary
            font.pixelSize: Theme.fontSizeSmall
            font.letterSpacing: 1.4
            font.capitalization: Font.AllUppercase
            elide: Text.ElideRight
        }

        Item {
            id: bodyItem
            anchors.top: titleText.bottom
            anchors.topMargin: titleText.visible ? 12 : 0
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }
}
