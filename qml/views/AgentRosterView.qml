pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Aegis 1.0
import "../theme"
import "../components"

Item {
    id: root

    property Item backdrop: null
    property string errorMessage: ""
    property bool retryable: false
    property string expandedAgentId: ""

    function refreshView() {
        errorMessage = "";
        agents.refresh();
    }

    function statusName(value) {
        return value === AgentStatus.Active ? qsTr("active") : value === AgentStatus.Idle ? qsTr("idle") : value === AgentStatus.Error ? qsTr("error") : qsTr("unknown");
    }

    function ringStatus(value) {
        return value === AgentStatus.Active ? StatusRing.Active : value === AgentStatus.Idle ? StatusRing.Idle : value === AgentStatus.Error ? StatusRing.Error : StatusRing.Unknown;
    }

    function initials(name) {
        const words = name.trim().split(/\s+/);
        if (words.length === 0 || words[0].length === 0)
            return "?";
        return (words[0].charAt(0) + (words.length > 1 ? words[words.length - 1].charAt(0) : words[0].charAt(1))).toUpperCase();
    }

    Connections {
        target: agents
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
            spacing: Theme.space.md

            Item {
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("FILTER")
                color: Theme.textMuted
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
                font.letterSpacing: Typography.caption.letterSpacing
            }
            ComboBox {
                id: filterBox
                model: [qsTr("All"), qsTr("Active"), qsTr("Idle"), qsTr("Error")]
                Accessible.name: qsTr("Filter agents by status")
                onActivated: agents.filterBy(currentIndex === 0 ? "all" : currentText.toLowerCase())
            }
        }

        GridView {
            id: rosterGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: agents.agents
            cellWidth: Math.max(Theme.minimumCardWidth + Theme.viewGutter, width / Math.max(1, Math.floor(width / (Theme.minimumCardWidth + Theme.viewGutter))))
            cellHeight: Theme.agentCardHeight + Theme.viewGutter + (root.expandedAgentId.length > 0 ? Theme.skeletonRowHeight : 0)

            delegate: Item {
                id: agentCell
                required property int index
                property string agentKey: displayName + "#" + index
                required property string displayName
                required property string model
                required property int status
                required property string statusDetail
                required property var lastSeen
                required property int activeSessions
                required property string avatarSeed
                width: rosterGrid.cellWidth - Theme.viewGutter
                height: root.expandedAgentId === agentCell.agentKey ? Theme.agentCardHeight + Theme.skeletonRowHeight : Theme.agentCardHeight

                GlassCard {
                    anchors.fill: parent
                    backdrop: root.backdrop
                    liveTracking: true
                    enterDelay: index * Motion.stagger
                    interactive: true

                    ColumnLayout {
                        width: parent.width
                        spacing: Theme.space.md

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.space.md

                            StatusRing {
                                status: root.ringStatus(agentCell.status)
                                size: Theme.ringDefaultSize

                                Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    color: agentCell.status === AgentStatus.Active ? Theme.accentSoft : agentCell.status === AgentStatus.Error ? Theme.alertSoft : Theme.warnSoft
                                    Text {
                                        anchors.centerIn: parent
                                        text: root.initials(agentCell.displayName)
                                        color: Theme.textPrimary
                                        font.family: Typography.heading.family
                                        font.pixelSize: Typography.heading.pixelSize
                                        font.weight: Typography.heading.weight
                                    }
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs
                                Text {
                                    Layout.fillWidth: true
                                    text: agentCell.displayName
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    font.family: Typography.heading.family
                                    font.pixelSize: Typography.heading.pixelSize
                                    font.weight: Typography.heading.weight
                                }
                                Text {
                                    text: root.statusName(agentCell.status)
                                    color: agentCell.status === AgentStatus.Active ? Theme.accent : agentCell.status === AgentStatus.Idle ? Theme.warn : agentCell.status === AgentStatus.Error ? Theme.alert : Theme.textMuted
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("model: %1").arg(agentCell.model.length > 0 ? agentCell.model : qsTr("n/a"))
                            color: Theme.textSecondary
                            elide: Text.ElideMiddle
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                        Text {
                            text: qsTr("sessions: %1").arg(agentCell.activeSessions)
                            color: Theme.textSecondary
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                        Text {
                            text: qsTr("last seen: %1").arg(agentCell.lastSeen ? Qt.formatDateTime(agentCell.lastSeen, "MMM d  HH:mm") : qsTr("never"))
                            color: Theme.textMuted
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: agentCell.status === AgentStatus.Error && agentCell.statusDetail.length > 0
                            text: agentCell.statusDetail
                            color: Theme.alert
                            wrapMode: Text.Wrap
                            maximumLineCount: root.expandedAgentId === agentCell.agentKey ? 4 : 1
                            elide: Text.ElideRight
                            font.family: Typography.body.family
                            font.pixelSize: Typography.body.pixelSize
                            font.weight: Typography.body.weight
                        }
                        GhostButton {
                            text: root.expandedAgentId === agentCell.agentKey ? qsTr("Hide details") : qsTr("Details")
                            Accessible.name: qsTr("Toggle details for %1").arg(agentCell.displayName)
                            onClicked: root.expandedAgentId = root.expandedAgentId === agentCell.agentKey ? "" : agentCell.agentKey
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: rosterGrid.count === 0 && !agents.loading && root.errorMessage.length === 0
                title: qsTr("No agents match this filter")
                detail: qsTr("Choose another status or refresh the live roster.")
                actionText: qsTr("Show all")
                onActionTriggered: {
                    filterBox.currentIndex = 0;
                    agents.filterBy("all");
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: agents.loading && rosterGrid.count === 0
        skeletonCount: Theme.defaultSkeletonCount + 2
    }

    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
