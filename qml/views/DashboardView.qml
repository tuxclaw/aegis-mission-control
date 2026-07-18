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
    signal openAgentsRequested

    readonly property string errorMessage: Agents.error
    readonly property bool retryable: Agents.error.length > 0
    readonly property bool loading: !Agents.available

    function refreshView() {
        Agents.refresh();
        Containers.refresh();
        app.refreshAll();
    }

    function statusName(value) {
        const state = String(value).toLowerCase();
        if (state === "active")
            return qsTr("active");
        if (state === "idle")
            return qsTr("idle");
        if (state === "error")
            return qsTr("error");
        return qsTr("unknown");
    }

    function ringStatus(value) {
        const state = String(value).toLowerCase();
        if (state === "active")
            return StatusRing.Active;
        if (state === "idle")
            return StatusRing.Idle;
        if (state === "error")
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

    function formatPercent(value) {
        return hasFiniteValue(value) ? qsTr("%1%").arg(Math.max(0, value).toFixed(1)) : qsTr("n/a");
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
                        backdrop: root.backdrop
                        liveTracking: true
                        required property int index
                        enterDelay: index * Motion.stagger
                        Layout.fillWidth: true
                        Layout.minimumWidth: Theme.minimumCardWidth
                        Layout.preferredHeight: Theme.gaugeSize + Theme.skeletonRowHeight + Theme.cardPadding * 3

                        VitalGauge {
                            anchors.horizontalCenter: parent.horizontalCenter
                            label: vitalCard.index === 0 ? qsTr("CPU") : vitalCard.index === 1 ? qsTr("GPU") : vitalCard.index === 2 ? qsTr("Memory") : qsTr("Network")
                            value: vitalCard.index === 0 ? Stats.cpuUsage * 100 : vitalCard.index === 1 ? Stats.gpuBusy * 100 : vitalCard.index === 2 && Stats.memTotalGiB > 0 ? Stats.memUsedGiB / Stats.memTotalGiB * 100 : Number.NaN
                            available: vitalCard.index !== 2 || Stats.memTotalGiB > 0
                            subtext: vitalCard.index === 1 && Stats.gpuTemp > 0 ? qsTr("%1 °C").arg(Stats.gpuTemp.toFixed(1)) : vitalCard.index === 2 ? qsTr("%1 / %2").arg(Stats.memUsedGiB.toFixed(1) + " GiB").arg(Stats.memTotalGiB.toFixed(1) + " GiB") : vitalCard.index === 3 ? qsTr("live throughput") : vitalCard.index === 0 ? qsTr("system load") : ""
                            accentColor: vitalCard.index === 2 ? Theme.ok : Theme.accent
                        }

                        RowLayout {
                            width: parent.width
                            height: vitalCard.index === 3 ? Theme.skeletonRowHeight : 0
                            visible: vitalCard.index === 3
                            Text {
                                Layout.fillWidth: true
                                text: qsTr("aggregate")
                                color: Theme.textMuted
                                elide: Text.ElideRight
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                            Text {
                                text: qsTr("↑ %1  ↓ %2").arg(root.formatRate(Stats.netTx)).arg(root.formatRate(Stats.netRx))
                                color: Theme.textSecondary
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: width >= Theme.minimumWindowWidth ? 2 : 1
                columnSpacing: Theme.viewGutter
                rowSpacing: Theme.viewGutter

                GlassCard {
                    title: qsTr("CONTAINERS")
                    backdrop: root.backdrop
                    liveTracking: true
                    enterDelay: 6 * Motion.stagger
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.minimumCardWidth
                    Layout.minimumHeight: Theme.contentMinimumHeight
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, containerList.contentHeight + Theme.cardPadding * 2)

                    ListView {
                        id: containerList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: Containers.items
                        spacing: Theme.space.md
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: RowLayout {
                            id: containerRow
                            required property var modelData
                            readonly property string containerName: String(modelData.name || "")
                            readonly property string imageName: String(modelData.image || "")
                            readonly property string containerState: String(modelData.state || "")
                            readonly property string statusText: String(modelData.status || "")
                            width: containerList.width
                            spacing: Theme.space.md

                            Rectangle {
                                Layout.alignment: Qt.AlignVCenter
                                implicitWidth: Theme.statusDotSize
                                implicitHeight: Theme.statusDotSize
                                radius: Theme.radiusPill
                                color: containerRow.containerState.toLowerCase() === "running" || containerRow.statusText.toLowerCase().startsWith("up") ? Theme.ok : Theme.textMuted
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs

                                Text {
                                    Layout.fillWidth: true
                                    text: containerRow.containerName
                                    textFormat: Text.PlainText
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    font.family: Typography.heading.family
                                    font.pixelSize: Typography.heading.pixelSize
                                    font.weight: Typography.heading.weight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: containerRow.imageName
                                    textFormat: Text.PlainText
                                    color: Theme.textSecondary
                                    elide: Text.ElideMiddle
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }

                            Text {
                                Layout.preferredWidth: Theme.splitPaneMinimumWidth / 2
                                visible: containerRow.statusText.length > 0
                                text: containerRow.statusText
                                textFormat: Text.PlainText
                                color: Theme.textMuted
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideMiddle
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: containerList.count === 0 && Boolean(Containers.available)
                            title: qsTr("No containers found")
                            detail: qsTr("Containers will appear here when they are available.")
                        }

                        LoadingState {
                            anchors.fill: parent
                            visible: !Containers.available
                        }
                    }
                }

                GlassCard {
                    title: qsTr("PROCESSES")
                    backdrop: root.backdrop
                    liveTracking: true
                    enterDelay: 7 * Motion.stagger
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.minimumCardWidth
                    Layout.minimumHeight: Theme.contentMinimumHeight
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, processList.contentHeight + Theme.cardPadding * 2)

                    ListView {
                        id: processList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: Stats.topProcesses
                        spacing: Theme.space.md
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: RowLayout {
                            id: processRow
                            required property var modelData
                            readonly property int processPid: Number(modelData.pid || 0)
                            readonly property string processName: String(modelData.name || "")
                            readonly property real processCpu: Number(modelData.cpu || 0)
                            readonly property real processMemMiB: Number(modelData.memMiB || 0)
                            width: processList.width
                            spacing: Theme.space.md

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs

                                Text {
                                    Layout.fillWidth: true
                                    text: processRow.processName
                                    textFormat: Text.PlainText
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    font.family: Typography.label.family
                                    font.pixelSize: Typography.label.pixelSize
                                    font.weight: Typography.label.weight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("PID %1").arg(processRow.processPid)
                                    textFormat: Text.PlainText
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: Theme.tableActionWidth / 2
                                spacing: Theme.space.xs

                                Text {
                                    Layout.fillWidth: true
                                    text: root.formatPercent(processRow.processCpu * 100)
                                    color: !root.hasFiniteValue(processRow.processCpu) ? Theme.textMuted : processRow.processCpu > 0.5 ? Theme.alert : processRow.processCpu > 0.25 ? Theme.warn : Theme.ok
                                    horizontalAlignment: Text.AlignRight
                                    font.family: Typography.data.family
                                    font.pixelSize: Typography.data.pixelSize
                                    font.weight: Typography.data.weight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("CPU")
                                    color: Theme.textMuted
                                    horizontalAlignment: Text.AlignRight
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }

                            ColumnLayout {
                                Layout.preferredWidth: Theme.tableActionWidth / 2
                                spacing: Theme.space.xs

                                Text {
                                    Layout.fillWidth: true
                                    text: root.hasFiniteValue(processRow.processMemMiB) ? qsTr("%1 MiB").arg(processRow.processMemMiB.toFixed(1)) : qsTr("n/a")
                                    color: root.hasFiniteValue(processRow.processMemMiB) ? Theme.textSecondary : Theme.textMuted
                                    horizontalAlignment: Text.AlignRight
                                    font.family: Typography.data.family
                                    font.pixelSize: Typography.data.pixelSize
                                    font.weight: Typography.data.weight
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: qsTr("MEM")
                                    color: Theme.textMuted
                                    horizontalAlignment: Text.AlignRight
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }
                        }

                        EmptyState {
                            anchors.centerIn: parent
                            visible: processList.count === 0
                            title: qsTr("No processes found")
                            detail: qsTr("Process activity will appear after the next refresh.")
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
                    backdrop: root.backdrop
                    liveTracking: true
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
                    backdrop: root.backdrop
                    liveTracking: true
                    enterDelay: 5 * Motion.stagger
                    Layout.fillWidth: true
                    Layout.minimumWidth: Theme.splitPaneMinimumWidth
                    Layout.preferredHeight: Math.max(Theme.contentMinimumHeight, fleetList.contentHeight + Theme.cardPadding * 2)
                    interactive: true

                    ListView {
                        id: fleetList
                        width: parent.width
                        height: Math.max(Theme.contentMinimumHeight - Theme.cardPadding * 2, contentHeight)
                        model: Agents.items
                        spacing: Theme.space.sm
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: RowLayout {
                            id: agentRow
                            required property var modelData
                            readonly property string agentName: String(modelData.name || "")
                            readonly property string agentModel: String(modelData.model || "")
                            readonly property string agentState: String(modelData.state || "")
                            width: fleetList.width
                            spacing: Theme.space.md
                            Accessible.name: qsTr("Open agent roster for %1").arg(agentRow.agentName)
                            Accessible.role: Accessible.Button

                            StatusRing {
                                size: Theme.compactButtonSize
                                status: root.ringStatus(agentRow.agentState)
                            }
                            Text {
                                Layout.fillWidth: true
                                text: agentRow.agentName
                                color: Theme.textPrimary
                                elide: Text.ElideRight
                                font.family: Typography.label.family
                                font.pixelSize: Typography.label.pixelSize
                                font.weight: Typography.label.weight
                            }
                            Text {
                                text: root.statusName(agentRow.agentState)
                                color: agentRow.agentState.toLowerCase() === "active" ? Theme.accent : agentRow.agentState.toLowerCase() === "idle" ? Theme.warn : agentRow.agentState.toLowerCase() === "error" ? Theme.alert : Theme.textMuted
                                font.family: Typography.caption.family
                                font.pixelSize: Typography.caption.pixelSize
                                font.weight: Typography.caption.weight
                            }
                            Text {
                                Layout.preferredWidth: Theme.splitPaneMinimumWidth / 3
                                text: agentRow.agentModel.length > 0 ? agentRow.agentModel : qsTr("n/a")
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
        visible: root.loading && root.errorMessage.length === 0 && fleetList.count === 0
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
