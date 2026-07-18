import QtQuick

/*
    GlassListRow — delegate base: hover highlight, hairline separator,
    content area with side margins. Lay out children inside with anchors.

    delegate: GlassListRow {
        required property var modelData
        Text { anchors.verticalCenter: parent.verticalCenter; text: modelData.name }
    }
*/
Item {
    id: root

    width: ListView.view ? ListView.view.width : implicitWidth
    height: 44

    readonly property bool hovered: hoverHandler.hovered
    default property alias content: inner.data

    HoverHandler { id: hoverHandler }

    Rectangle {
        anchors.fill: parent
        anchors.bottomMargin: 1
        radius: Theme.radiusSmall
        color: Qt.rgba(1, 1, 1, 0.05)
        visible: root.hovered
    }

    Item {
        id: inner
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Qt.rgba(1, 1, 1, 0.07)
    }
}
