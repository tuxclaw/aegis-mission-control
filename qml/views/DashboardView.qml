pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Aegis 1.0
import "../theme"
import "../components"

Item {
    id: root

    property string errorMessage: ""
    property bool retryable: false
    readonly property bool loading: agents.loading

    function refreshView() {
        errorMessage = "";
        agents.refresh();
        app.refreshAll();
    }

    function statusName(value) {
        if (value === AgentStatus.Active)
            return qsTr("active");
        if (value === AgentStatus.Idle)
            return qsTr("idle");
        if (value === AgentStatus.Error)
            return qsTr("error");
        return qsTr("unknown");
    }

    function ringStatus(value) {
        if (value === AgentStatus.Active)
            return StatusRing.Active;
        if (value === AgentStatus.Idle)
            return StatusRing.Idle;
        if (value === AgentStatus.Error)
            return StatusRing.Error;
        return StatusRing.Unknown;
    }

    Connections {
        target: agents
        function onErrorRaised(message, canRetry) {
            root.errorMessage = message;
            root.retryable = canRetry;
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: Theme.viewGutter

            GridLayout {
                Layout.fillWidth: true
                columns: width >= Theme.minimumWindowWidth ? 4 : 2
                columnSpacing: Theme.viewGutter
                rowSpacing: Theme.viewGutter

                Repeater {
                    model: 4

                    GlassCard {
                        id: vitalCard
                        required property int index
                        Layout.fillWidth: true
                        Layout.minimumWidth: Theme.minimumCardWidth
                        Layout.preferredHeight: Theme.gaugeSize + Theme.cardPadding * 3

                        VitalGauge {
                            anchors.horizontalCenter: parent.horizontalCenter
                            label: vitalCard.index === 0 ? qsTr("CPU") : vitalCard.index === 1 ? qsTr("GPU") : vitalCard.index === 2 ? qsTr("Memory") : qsTr("Network")
                            value: vitalCard.index === 0 ? vitals.cpuPct : vitalCard.index === 1 ? vitals.gpuPct : vitalCard.index === 2 ? vitals.memPct : vitals.netModel.count > 0 ? vitals.netModel.aggregatePct : Number.NaN
                            available: vitalCard.index !== 1 || !Number.isNaN(vitals.gpuPct)
                            subtext: vitalCard.index === 1 ? vitals.gpuVendor : vitalCard.index === 2 ? qsTr("%1 / %2").arg(vitals.memUsed).arg(vitals.memTotal) : vitalCard.index === 3 ? qsTr("live throughput") : qsTr("system load")
                            accentColor: vitalCard.index === 2 ? Theme.ok : Theme.accent
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width >= Theme.sidebarAutoCollapseWidth ? 2 : 1
                columnSpacing: Theme.viewGutter
                rowSpacing: Theme.viewGutter

                GlassCard {
                    title: qsTr("DISKS")
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.splitPaneMinimumWidth
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, diskList.contentHeight + Theme.cardPadding * 2)

                    ListView {
                        id: diskList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: vitals.diskModel
                        spacing: Theme.space.lg
                        clip: true

                        delegate: Column {
                            id: diskRow
                            required property string mount
                            required property real usedBytes
                            required property real totalBytes
                            width: diskList.width
                            spacing: Theme.space.sm
                            readonly property real percent: totalBytes > 0 ? usedBytes / totalBytes * 100 : 0

                            RowLayout {
                                width: parent.width
                                Text {
                                    Layout.fillWidth: true
                                    text: diskRow.mount
                                    color: Theme.textPrimary
                                    elide: Text.ElideMiddle
                                    font.family: Typography.data.family
                                    font.pixelSize: Typography.data.pixelSize
                                    font.weight: Typography.data.weight
                                }
                                Text {
                                    text: qsTr("%1%").arg(Math.round(diskRow.percent))
                                    color: Theme.textSecondary
                                    font.family: Typography.dataSmall.family
                                    font.pixelSize: Typography.dataSmall.pixelSize
                                    font.weight: Typography.dataSmall.weight
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: Theme.usageBarHeight
                                radius: Theme.radiusPill
                                color: Theme.skeletonBase
                                Rectangle {
                                    width: parent.width * Math.max(0, Math.min(1, diskRow.percent / 100))
                                    height: parent.height
                                    radius: parent.radius
                                    color: diskRow.percent >= 90 ? Theme.alert : diskRow.percent >= 75 ? Theme.warn : Theme.ok
                                    Behavior on width { NumberAnimation { duration: Motion.gauge; easing.type: Motion.gaugeEasing } }
                                }
                            }
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: diskList.count === 0
                            title: qsTr("No disk data")
                            detail: qsTr("Disk usage will appear after the next vitals sample.")
                        }
                    }
                }

                GlassCard {
                    title: qsTr("FLEET AT A GLANCE")
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.splitPaneMinimumWidth
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, fleetList.contentHeight + Theme.cardPadding * 2)
                    interactive: true

                    ListView {
                        id: fleetList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: agents.agents
                        spacing: Theme.space.sm
                        clip: true

                        delegate: RowLayout {
                            id: agentRow
                            required property string displayName
                            required property string model
                            required property int status
                            width: fleetList.width
                            spacing: Theme.space.md

                            StatusRing {
                                size: Theme.compactButtonSize
                                status: root.ringStatus(agentRow.status)
                            }
                            Text {
                                Layout.fillWidth: true
                                text: agentRow.displayName
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                font.family: Typography.label.family
                                font.pixelSize: Typography.label.pixelSize
                                font.weight: Typography.label.weight
                            }
                            Text {
                                text: root.statusName(agentRow.status)
                                color: agentRow.status === AgentStatus.Active ? Theme.accent : agentRow.status === AgentStatus.Idle ? Theme.warn : agentRow.status === AgentStatus.Error ? Theme.alert : Theme.textMuted
                                font.family: Typography.caption.family
                                font.pixelSize: Typography.caption.pixelSize
                                font.weight: Typography.caption.weight
                            }
                            Text {
                                Layout.preferredWidth: Theme.splitPaneMinimumWidth / 3
                                text: agentRow.model.length > 0 ? agentRow.model : qsTr("n/a")
                                color: Theme.textSecondary
                                elide: Text.ElideMiddle
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: fleetList.count === 0 && !root.loading
                            title: qsTr("No agents reported")
                            actionText: qsTr("Refresh")
                            onActionTriggered: root.refreshView()
                        }
                    }
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: root.loading && fleetList.count === 0
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
