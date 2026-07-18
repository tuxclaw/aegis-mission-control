pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Aegis 1.0
import "../theme"
import "../components"

Item {
    id: root

    signal openAgentsRequested

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

    function formatRate(bytes) {
        if (bytes >= 1024 * 1024)
            return qsTr("%1 MB/s").arg((bytes / (1024 * 1024)).toFixed(1));
        if (bytes >= 1024)
            return qsTr("%1 KB/s").arg((bytes / 1024).toFixed(1));
        return qsTr("%1 B/s").arg(Math.round(bytes));
    }

    function hasFiniteValue(value) {
        return typeof value === "number" && Number.isFinite(value);
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
        Component.onCompleted: contentItem["boundsBehavior"] = Flickable.StopAtBounds

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
                        enterDelay: index * Motion.stagger
                        Layout.fillWidth: true
                        Layout.minimumWidth: Theme.minimumCardWidth
                        Layout.preferredHeight: Theme.gaugeSize + Theme.skeletonRowHeight + Theme.cardPadding * 3

                        VitalGauge {
                            anchors.horizontalCenter: parent.horizontalCenter
                            label: vitalCard.index === 0 ? qsTr("CPU") : vitalCard.index === 1 ? qsTr("GPU") : vitalCard.index === 2 ? qsTr("Memory") : qsTr("Network")
                            value: vitalCard.index === 0 ? vitals.cpuPct : vitalCard.index === 1 ? vitals.gpuPct : vitalCard.index === 2 ? vitals.memPct : Number.NaN
                            available: vitalCard.index === 1 ? root.hasFiniteValue(vitals.gpuPct) : vitalCard.index === 3 ? vitals.netModel !== null : true
                            subtext: vitalCard.index === 1 && root.hasFiniteValue(vitals.gpuPct) ? vitals.gpuVendor : vitalCard.index === 2 ? qsTr("%1 / %2").arg(vitals.memUsed).arg(vitals.memTotal) : vitalCard.index === 3 ? qsTr("live throughput") : vitalCard.index === 0 ? qsTr("system load") : ""
                            accentColor: vitalCard.index === 2 ? Theme.ok : Theme.accent
                        }

                        ListView {
                            width: parent.width
                            height: vitalCard.index === 3 ? Theme.skeletonRowHeight : 0
                            visible: vitalCard.index === 3
                            model: vitals.netModel
                            boundsBehavior: Flickable.StopAtBounds
                            clip: true
                            delegate: RowLayout {
                                id: networkRow
                                required property string iface
                                required property real rxBytesPerSec
                                required property real txBytesPerSec
                                width: ListView.view.width
                                Text {
                                    Layout.fillWidth: true
                                    text: networkRow.iface
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                    font.family: Typography.dataSmall.family
                                    font.pixelSize: Typography.dataSmall.pixelSize
                                    font.weight: Typography.dataSmall.weight
                                }
                                Text {
                                    text: qsTr("↑ %1  ↓ %2").arg(root.formatRate(networkRow.txBytesPerSec)).arg(root.formatRate(networkRow.rxBytesPerSec))
                                    color: Theme.textSecondary
                                    font.family: Typography.dataSmall.family
                                    font.pixelSize: Typography.dataSmall.pixelSize
                                    font.weight: Typography.dataSmall.weight
                                }
                            }
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
                    enterDelay: 4 * Motion.stagger
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.splitPaneMinimumWidth
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, diskList.contentHeight + Theme.cardPadding * 2)

                    ListView {
                        id: diskList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: vitals.diskModel
                        spacing: Theme.space.lg
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: Column {
                            id: diskRow
                            required property string mount
                            required property real usedBytes
                            required property real totalBytes
                            width: diskList.width
                            spacing: Theme.space.sm
                            readonly property bool hasData: mount.length > 0 && root.hasFiniteValue(totalBytes) && root.hasFiniteValue(usedBytes) && totalBytes > 0 && usedBytes >= 0
                            readonly property real percent: hasData ? usedBytes / totalBytes * 100 : 0

                            RowLayout {
                                width: parent.width
                                Text {
                                    Layout.fillWidth: true
                                    text: diskRow.hasData ? diskRow.mount : qsTr("n/a")
                                    color: diskRow.hasData ? Theme.textPrimary : Theme.textMuted
                                    elide: Text.ElideMiddle
                                    font.family: Typography.data.family
                                    font.pixelSize: Typography.data.pixelSize
                                    font.weight: Typography.data.weight
                                }
                                Text {
                                    text: diskRow.hasData ? qsTr("%1%").arg(Math.round(diskRow.percent)) : qsTr("n/a")
                                    color: diskRow.hasData ? Theme.textSecondary : Theme.textMuted
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
                                opacity: diskRow.hasData ? 1 : 0.5
                                Rectangle {
                                    width: diskRow.hasData ? parent.width * Math.max(0, Math.min(1, diskRow.percent / 100)) : 0
                                    height: parent.height
                                    radius: parent.radius
                                    color: diskRow.percent >= 90 ? Theme.alert : diskRow.percent >= 75 ? Theme.warn : Theme.ok
                                    Behavior on width {
                                        NumberAnimation {
                                            duration: Motion.gauge
                                            easing.type: Motion.gaugeEasing
                                        }
                                    }
                                }
                            }
                        }

                        Text {
                            anchors.centerIn: parent
                            visible: diskList.count === 0
                            text: qsTr("n/a")
                            color: Theme.textMuted
                            font.family: Typography.data.family
                            font.pixelSize: Typography.data.pixelSize
                            font.weight: Typography.data.weight
                        }
                    }
                }

                GlassCard {
                    title: qsTr("FLEET AT A GLANCE")
                    enterDelay: 5 * Motion.stagger
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
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: RowLayout {
                            id: agentRow
                            required property string displayName
                            required property string model
                            required property int status
                            width: fleetList.width
                            spacing: Theme.space.md
                            Accessible.name: qsTr("Open agent roster for %1").arg(agentRow.displayName)
                            Accessible.role: Accessible.Button

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
                            TapHandler {
                                onTapped: root.openAgentsRequested()
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
