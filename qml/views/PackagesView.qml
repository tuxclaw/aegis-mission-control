pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root

    property Item backdrop: null
    property string errorMessage: ""
    property bool retryable: false

    function refreshView() {
        errorMessage = "";
        packages.refresh();
    }

    Connections {
        target: packages
        function onErrorRaised(message, canRetry) {
            root.errorMessage = message;
            root.retryable = canRetry;
        }
        function onToast(message, level) {
            ToastHost.show(message, level);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.viewGutter

        RowLayout {
            Layout.fillWidth: true
            Item {
                Layout.fillWidth: true
            }
            TextField {
                id: filterField
                Layout.preferredWidth: Theme.splitPaneMinimumWidth
                placeholderText: qsTr("Filter package names")
                Accessible.name: qsTr("Filter packages by name")
                onTextChanged: packages.filterBy(text)
            }
        }

        GlassCard {
            backdrop: root.backdrop
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.tableHeaderHeight
                    color: Theme.bgElevated
                    border.width: Theme.borderWidth
                    border.color: Theme.divider
                    radius: Theme.radiusCard
                    clip: true
                    RowLayout {
                        anchors {
                            fill: parent
                            leftMargin: Theme.space.lg
                            rightMargin: Theme.space.lg
                        }
                        spacing: Theme.space.md
                        Text {
                            Layout.preferredWidth: parent.width * 0.45
                            text: qsTr("NAME")
                            color: Theme.textMuted
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                        }
                        Text {
                            Layout.preferredWidth: parent.width * 0.35
                            text: qsTr("VERSION")
                            color: Theme.textMuted
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("SOURCE")
                            color: Theme.textMuted
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                        }
                    }
                }

                TableView {
                    id: packageTable
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    model: packages.packages
                    rowSpacing: Theme.borderWidth

                    delegate: Rectangle {
                        id: packageRow
                        required property string name
                        required property string version
                        required property string source
                        implicitWidth: packageTable.width
                        implicitHeight: Theme.tableRowHeight
                        color: packageHover.hovered ? Theme.accentSoft : Theme.transparent

                        RowLayout {
                            anchors {
                                fill: parent
                                leftMargin: Theme.space.lg
                                rightMargin: Theme.space.lg
                            }
                            spacing: Theme.space.md
                            Text {
                                Layout.preferredWidth: parent.width * 0.45
                                text: packageRow.name
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                font.family: Typography.data.family
                                font.pixelSize: Typography.data.pixelSize
                                font.weight: Typography.data.weight
                            }
                            Text {
                                Layout.preferredWidth: parent.width * 0.35
                                text: packageRow.version
                                color: Theme.textSecondary
                                elide: Text.ElideMiddle
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: packageRow.source
                                color: Theme.textMuted
                                elide: Text.ElideRight
                                font.family: Typography.caption.family
                                font.pixelSize: Typography.caption.pixelSize
                                font.weight: Typography.caption.weight
                            }
                        }
                        HoverHandler {
                            id: packageHover
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Theme.tableHeaderHeight
                    color: Theme.bgElevated
                    border.width: Theme.borderWidth
                    border.color: Theme.divider
                    radius: Theme.radiusCard
                    clip: true
                    Text {
                        anchors {
                            left: parent.left
                            verticalCenter: parent.verticalCenter
                            leftMargin: Theme.space.lg
                        }
                        text: qsTr("%1 packages · read-only inventory").arg(packages.count)
                        color: Theme.textMuted
                        font.family: Typography.dataSmall.family
                        font.pixelSize: Typography.dataSmall.pixelSize
                        font.weight: Typography.dataSmall.weight
                    }
                }
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !packages.loading && packageTable.rows === 0 && root.errorMessage.length === 0
        title: filterField.text.length > 0 ? qsTr("No matching packages") : qsTr("No packages reported")
        detail: filterField.text.length > 0 ? qsTr("Try a shorter name filter.") : qsTr("The package inventory is empty.")
        actionText: filterField.text.length > 0 ? qsTr("Clear filter") : qsTr("Refresh")
        onActionTriggered: filterField.text.length > 0 ? filterField.clear() : root.refreshView()
    }
    LoadingState {
        anchors.fill: parent
        visible: packages.loading && packageTable.rows === 0
    }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0 && packageTable.rows === 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
