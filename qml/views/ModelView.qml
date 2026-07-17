pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "../theme"
import "../components"

Item {
    id: root

    property string errorMessage: ""
    property bool retryable: false
    property string pendingModelId: ""

    function refreshView() {
        errorMessage = "";
        models.refresh();
    }

    function activate(modelId) {
        if (modelId.length === 0 || modelGrid.count === 0) {
            ToastHost.error(qsTr("That model is no longer available."));
            return;
        }
        pendingModelId = modelId;
        models.setActiveModel(modelId);
    }

    Connections {
        target: models
        function onErrorRaised(message, canRetry) {
            root.pendingModelId = "";
            root.errorMessage = message;
            root.retryable = canRetry;
            ToastHost.error(message);
        }
        function onToast(message, level) {
            root.pendingModelId = "";
            ToastHost.show(message, level);
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.viewGutter

        GlassCard {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.tableHeaderHeight + Theme.cardPadding * 2

            RowLayout {
                width: parent.width
                spacing: Theme.space.md
                Text {
                    text: qsTr("ACTIVE MODEL")
                    color: Theme.textMuted
                    font.family: Typography.caption.family
                    font.pixelSize: Typography.caption.pixelSize
                    font.weight: Typography.caption.weight
                    font.letterSpacing: Typography.caption.letterSpacing
                }
                Text {
                    Layout.fillWidth: true
                    text: models.activeModel.length > 0 ? models.activeModel : qsTr("No active model reported")
                    color: models.activeModel.length > 0 ? Theme.accent : Theme.textMuted
                    elide: Text.ElideMiddle
                    font.family: Typography.data.family
                    font.pixelSize: Typography.data.pixelSize
                    font.weight: Typography.data.weight
                }
            }
        }

        GridView {
            id: modelGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: models.models
            cellWidth: Math.max(Theme.minimumCardWidth + Theme.viewGutter, width / Math.max(1, Math.floor(width / (Theme.minimumCardWidth + Theme.viewGutter))))
            cellHeight: Theme.modelCardHeight + Theme.viewGutter

            delegate: Item {
                id: modelCell
                required property int index
                required property var model
                property string modelId: model.id
                required property string provider
                required property string label
                required property bool isActive
                readonly property bool activeCard: isActive || modelId === models.activeModel
                width: modelGrid.cellWidth - Theme.viewGutter
                height: Theme.modelCardHeight

                Rectangle {
                    anchors.fill: parent
                    radius: Theme.radiusCard
                    color: Theme.transparent
                    border.width: modelCell.activeCard ? Theme.focusBorderWidth : 0
                    border.color: Theme.accent
                    opacity: modelCell.activeCard ? 1 : 0
                    layer.enabled: modelCell.activeCard
                    layer.effect: MultiEffect {
                        blurEnabled: true
                        blur: 1
                        blurMax: Theme.glowBlur
                    }
                }

                GlassCard {
                    anchors {
                        fill: parent
                        margins: modelCell.activeCard ? Theme.borderWidth : 0
                    }
                    interactive: !modelCell.activeCard
                    enterDelay: (index + 1) * Motion.stagger

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: "⬢"
                                color: modelCell.activeCard ? Theme.accent : Theme.textSecondary
                                font.family: Typography.heading.family
                                font.pixelSize: Typography.heading.pixelSize
                                font.weight: Typography.heading.weight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelCell.label.length > 0 ? modelCell.label : modelCell.modelId
                                color: Theme.textPrimary
                                elide: Text.ElideMiddle
                                font.family: Typography.heading.family
                                font.pixelSize: Typography.heading.pixelSize
                                font.weight: Typography.heading.weight
                            }
                            Rectangle {
                                visible: modelCell.activeCard
                                Layout.preferredWidth: activeLabel.implicitWidth + Theme.space.lg
                                Layout.preferredHeight: Theme.chipHeight
                                radius: Theme.radiusPill
                                color: Theme.accentSoft
                                Text {
                                    id: activeLabel
                                    anchors.centerIn: parent
                                    text: qsTr("ACTIVE")
                                    color: Theme.accent
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: modelCell.provider
                            color: Theme.textSecondary
                            elide: Text.ElideRight
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            Item {
                                Layout.fillWidth: true
                            }
                            SecondaryButton {
                                visible: !modelCell.activeCard
                                text: qsTr("Set active")
                                busy: root.pendingModelId === modelCell.modelId
                                enabled: root.pendingModelId.length === 0
                                Accessible.name: qsTr("Set %1 active").arg(modelCell.label)
                                onClicked: root.activate(modelCell.modelId)
                            }
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: modelGrid.count === 0 && !models.loading && root.errorMessage.length === 0
                title: qsTr("No models available")
                detail: qsTr("The gateway did not report a live model list.")
                actionText: qsTr("Refresh")
                onActionTriggered: root.refreshView()
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: models.loading && modelGrid.count === 0
    }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0 && modelGrid.count === 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
