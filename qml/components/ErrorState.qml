import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: root

    property string userMessage: qsTr("Something went wrong")
    property bool retryable: false
    signal retryRequested

    implicitWidth: Theme.minimumCardWidth
    implicitHeight: contentRow.implicitHeight + Theme.cardPadding * 2
    color: Theme.alertSoft
    radius: Theme.radiusCard
    border.width: Theme.borderWidth
    border.color: Theme.alert

    RowLayout {
        id: contentRow
        anchors {
            fill: parent
            margins: Theme.cardPadding
        }
        spacing: Theme.space.lg

        Text {
            text: "!"
            color: Theme.alert
            font.family: Typography.display.family
            font.pixelSize: Typography.display.pixelSize
            font.weight: Typography.display.weight
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.space.sm

            Text {
                Layout.fillWidth: true
                text: qsTr("Unable to load")
                color: Theme.textPrimary
                font.family: Typography.heading.family
                font.pixelSize: Typography.heading.pixelSize
                font.weight: Typography.heading.weight
                font.letterSpacing: Typography.heading.letterSpacing
            }
            Text {
                Layout.fillWidth: true
                text: root.userMessage
                color: Theme.textSecondary
                wrapMode: Text.Wrap
                font.family: Typography.body.family
                font.pixelSize: Typography.body.pixelSize
                font.weight: Typography.body.weight
                font.letterSpacing: Typography.body.letterSpacing
            }
        }
        PrimaryButton {
            visible: root.retryable
            text: qsTr("Retry")
            Accessible.name: qsTr("Retry loading")
            onClicked: root.retryRequested()
        }
    }
}
