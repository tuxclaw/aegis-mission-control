import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Dialog {
    id: root

    property string action: qsTr("Confirm action")
    property string detail: ""
    property bool destructive: false

    modal: true
    focus: true
    width: Math.min(Theme.dialogWidth, parent ? parent.width - Theme.dialogMaxWidthMargin : Theme.dialogWidth)
    anchors.centerIn: parent
    padding: Theme.cardPadding
    closePolicy: Popup.CloseOnEscape

    Overlay.modal: Rectangle {
        color: Theme.modalBackdrop
    }

    background: Rectangle {
        color: Theme.panel
        radius: Theme.radiusCard
        border.width: Theme.borderWidth
        border.color: root.destructive ? Theme.alert : Theme.panelBorder
        Accessible.name: root.action
        Accessible.role: Accessible.Dialog
    }

    contentItem: ColumnLayout {
        spacing: Theme.space.lg

        Text {
            Layout.fillWidth: true
            text: root.action
            color: Theme.textPrimary
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
            color: Theme.textSecondary
            wrapMode: Text.Wrap
            font.family: Typography.body.family
            font.pixelSize: Typography.body.pixelSize
            font.weight: Typography.body.weight
            font.letterSpacing: Typography.body.letterSpacing
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space.md

            Item {
                Layout.fillWidth: true
            }
            GhostButton {
                id: cancelButton
                text: qsTr("Cancel")
                Accessible.name: qsTr("Cancel %1").arg(root.action)
                onClicked: root.reject()
            }
            PrimaryButton {
                text: qsTr("Confirm")
                destructive: root.destructive
                Accessible.name: qsTr("Confirm %1").arg(root.action)
                onClicked: root.accept()
            }
        }
    }

    onOpened: cancelButton.forceActiveFocus()
}
