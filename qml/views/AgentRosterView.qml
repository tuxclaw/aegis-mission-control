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
    readonly property string errorMessage: Agents.error
    readonly property bool retryable: Agents.error.length > 0
    property string expandedAgentId: ""
    property string stateFilter: "all"
    readonly property var filteredAgents: Agents.items.filter(function(item) {
        return root.stateFilter === "all" || String(item.state).toLowerCase() === root.stateFilter;
    })

    function refreshView() {
        Agents.refresh();
    }

    function statusName(value) {
        const state = String(value).toLowerCase();
        return state === "active" ? qsTr("active") : state === "idle" ? qsTr("idle") : state === "error" ? qsTr("error") : qsTr("unknown");
    }

    function ringStatus(value) {
        const state = String(value).toLowerCase();
        return state === "active" ? StatusRing.Active : state === "idle" ? StatusRing.Idle : state === "error" ? StatusRing.Error : StatusRing.Unknown;
    }

    function initials(name) {
        const words = name.trim().split(/\s+/);
        if (words.length === 0 || words[0].length === 0)
            return "?";
        return (words[0].charAt(0) + (words.length > 1 ? words[words.length - 1].charAt(0) : words[0].charAt(1))).toUpperCase();
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
                onActivated: root.stateFilter = currentIndex === 0 ? "all" : currentText.toLowerCase()
            }
        }

        GridView {
            id: rosterGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            model: root.filteredAgents
            cellWidth: Math.max(Theme.minimumCardWidth + Theme.viewGutter, width / Math.max(1, Math.floor(width / (Theme.minimumCardWidth + Theme.viewGutter))))
            cellHeight: Theme.agentCardHeight + Theme.viewGutter + (root.expandedAgentId.length > 0 ? Theme.skeletonRowHeight : 0)

            delegate: Item {
                id: agentCell
                required property int index
                required property var modelData
                readonly property string agentName: String(modelData.name || "")
                readonly property string agentModel: String(modelData.model || "")
                readonly property string agentState: String(modelData.state || "")
                readonly property string agentDetail: String(modelData.detail || "")
                property string agentKey: agentName + "#" + index
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
                                status: root.ringStatus(agentCell.agentState)
                                size: Theme.ringDefaultSize

                                Rectangle {
                                    anchors.fill: parent
                                    radius: width / 2
                                    color: agentCell.agentState.toLowerCase() === "active" ? Theme.accentSoft : agentCell.agentState.toLowerCase() === "error" ? Theme.alertSoft : Theme.warnSoft
                                    Text {
                                        anchors.centerIn: parent
                                        text: root.initials(agentCell.agentName)
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
                                    text: agentCell.agentName
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    font.family: Typography.heading.family
                                    font.pixelSize: Typography.heading.pixelSize
                                    font.weight: Typography.heading.weight
                                }
                                Text {
                                    text: root.statusName(agentCell.agentState)
                                    color: agentCell.agentState.toLowerCase() === "active" ? Theme.accent : agentCell.agentState.toLowerCase() === "idle" ? Theme.warn : agentCell.agentState.toLowerCase() === "error" ? Theme.alert : Theme.textMuted
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }
                        }

                        Text {
                            Layout.fillWidth: true
                            text: qsTr("model: %1").arg(agentCell.agentModel.length > 0 ? agentCell.agentModel : qsTr("n/a"))
                            color: Theme.textSecondary
                            elide: Text.ElideMiddle
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: agentCell.agentDetail.length > 0
                            text: agentCell.agentDetail
                            color: agentCell.agentState.toLowerCase() === "error" ? Theme.alert : Theme.textMuted
                            wrapMode: Text.Wrap
                            maximumLineCount: root.expandedAgentId === agentCell.agentKey ? 4 : 1
                            elide: Text.ElideRight
                            font.family: Typography.body.family
                            font.pixelSize: Typography.body.pixelSize
                            font.weight: Typography.body.weight
                        }
                        GhostButton {
                            text: root.expandedAgentId === agentCell.agentKey ? qsTr("Hide details") : qsTr("Details")
                            Accessible.name: qsTr("Toggle details for %1").arg(agentCell.agentName)
                            onClicked: root.expandedAgentId = root.expandedAgentId === agentCell.agentKey ? "" : agentCell.agentKey
                        }
                    }
                }
            }

            EmptyState {
                anchors.centerIn: parent
                visible: rosterGrid.count === 0 && Agents.available && root.errorMessage.length === 0
                title: qsTr("No agents match this filter")
                detail: qsTr("Choose another status or refresh the live roster.")
                actionText: qsTr("Show all")
                onActionTriggered: {
                    filterBox.currentIndex = 0;
                    root.stateFilter = "all";
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: !Agents.available && root.errorMessage.length === 0 && rosterGrid.count === 0
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
