import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: root

    property url icon
    property string title: qsTr("Nothing here yet")
    property string detail: ""
    property string actionText: ""
    signal actionTriggered

    implicitWidth: Theme.minimumCardWidth
    implicitHeight: contentColumn.implicitHeight

    ColumnLayout {
        id: contentColumn
        anchors.centerIn: parent
        width: Math.min(parent.width, Theme.dialogWidth)
        spacing: Theme.space.md

        Image {
            Layout.alignment: Qt.AlignHCenter
            visible: root.icon.toString().length > 0
            source: root.icon
            sourceSize.width: Theme.emptyStateIconSize
            sourceSize.height: Theme.emptyStateIconSize
            opacity: 0.55
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            visible: root.icon.toString().length === 0
            text: "◇"
            color: Theme.textMuted
            font.family: Typography.display.family
            font.pixelSize: Typography.display.pixelSize
            font.weight: Typography.display.weight
        }
        Text {
            Layout.fillWidth: true
            text: root.title
            color: Theme.textSecondary
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.family: Typography.heading.family
            font.pixelSize: Typography.heading.pixelSize
            font.weight: Typography.heading.weight
            font.letterSpacing: Typography.heading.letterSpacing
        }
        Text {
            Layout.fillWidth: true
            visible: root.detail.length > 0
            text: root.detail
            color: Theme.textMuted
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.family: Typography.body.family
            font.pixelSize: Typography.body.pixelSize
            font.weight: Typography.body.weight
            font.letterSpacing: Typography.body.letterSpacing
        }
        PrimaryButton {
            Layout.alignment: Qt.AlignHCenter
            visible: root.actionText.length > 0
            text: root.actionText
            Accessible.name: root.actionText
            onClicked: root.actionTriggered()
        }
    }
}
