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
    property string pendingJobId: ""

    function refreshView() {
        errorMessage = "";
        cron.refresh();
    }

    function runNow(jobId) {
        pendingJobId = jobId;
        cron.runJob(jobId);
    }

    function toggle(jobId, currentlyEnabled) {
        pendingJobId = jobId;
        cron.toggleJob(jobId, !currentlyEnabled);
    }

    Connections {
        target: cron
        function onErrorRaised(message, canRetry) {
            root.pendingJobId = "";
            root.errorMessage = message;
            root.retryable = canRetry;
            ToastHost.error(message);
        }
        function onToast(message, level) {
            root.pendingJobId = "";
            ToastHost.show(message, level);
        }
    }

    GlassCard {
        anchors.fill: parent
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

                RowLayout {
                    anchors { fill: parent; leftMargin: Theme.space.lg; rightMargin: Theme.space.lg }
                    spacing: Theme.space.md
                    Text { Layout.preferredWidth: parent.width * 0.25; text: qsTr("NAME"); color: Theme.textMuted; font: Typography.caption }
                    Text { Layout.preferredWidth: parent.width * 0.18; text: qsTr("SCHEDULE"); color: Theme.textMuted; font: Typography.caption }
                    Text { Layout.preferredWidth: parent.width * 0.12; text: qsTr("STATE"); color: Theme.textMuted; font: Typography.caption }
                    Text { Layout.preferredWidth: parent.width * 0.17; text: qsTr("LAST RUN"); color: Theme.textMuted; font: Typography.caption }
                    Text { Layout.fillWidth: true; text: qsTr("NEXT RUN"); color: Theme.textMuted; font: Typography.caption }
                    Text { Layout.preferredWidth: Theme.tableActionWidth; text: qsTr("ACTIONS"); color: Theme.textMuted; font: Typography.caption; horizontalAlignment: Text.AlignRight }
                }
            }

            TableView {
                id: jobTable
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: cron.jobs
                columnSpacing: 0
                rowSpacing: Theme.borderWidth

                delegate: Rectangle {
                    id: jobRow
                    property string jobId: model.id
                    required property string name
                    required property string schedule
                    required property int state
                    required property var lastRun
                    required property var nextRun
                    required property string lastResult
                    readonly property bool enabledState: state === CronState.Enabled
                    implicitWidth: jobTable.width
                    implicitHeight: Theme.tableRowHeight
                    color: rowTap.hovered ? Theme.accentSoft : Theme.transparent

                    RowLayout {
                        anchors { fill: parent; leftMargin: Theme.space.lg; rightMargin: Theme.space.lg }
                        spacing: Theme.space.md

                        Text { Layout.preferredWidth: parent.width * 0.25; text: jobRow.name; color: Theme.textPrimary; elide: Text.ElideRight; font: Typography.label }
                        Text { Layout.preferredWidth: parent.width * 0.18; text: jobRow.schedule; color: Theme.textSecondary; elide: Text.ElideRight; font: Typography.dataSmall }
                        Item {
                            Layout.preferredWidth: parent.width * 0.12
                            Layout.preferredHeight: Theme.chipHeight
                            Rectangle {
                                anchors.left: parent.left
                                width: stateLabel.implicitWidth + Theme.space.lg
                                height: Theme.chipHeight
                                radius: Theme.radiusPill
                                color: jobRow.enabledState ? Theme.okSoft : Theme.skeletonBase
                                Text {
                                    id: stateLabel
                                    anchors.centerIn: parent
                                    text: jobRow.enabledState ? qsTr("ON") : qsTr("OFF")
                                    color: jobRow.enabledState ? Theme.ok : Theme.textMuted
                                    font: Typography.caption
                                }
                            }
                        }
                        RowLayout {
                            Layout.preferredWidth: parent.width * 0.17
                            spacing: Theme.space.xs
                            Text { text: jobRow.lastResult.toLowerCase().indexOf("success") >= 0 ? "✓" : jobRow.lastResult.length > 0 ? "✕" : "—"; color: text === "✓" ? Theme.ok : text === "✕" ? Theme.alert : Theme.textMuted; font: Typography.data }
                            Text { Layout.fillWidth: true; text: jobRow.lastRun ? Qt.formatDateTime(jobRow.lastRun, "MMM d  HH:mm") : qsTr("Never"); color: Theme.textSecondary; elide: Text.ElideRight; font: Typography.dataSmall }
                        }
                        Text { Layout.fillWidth: true; text: jobRow.nextRun ? Qt.formatDateTime(jobRow.nextRun, "MMM d  HH:mm") : "—"; color: Theme.textSecondary; elide: Text.ElideRight; font: Typography.dataSmall }
                        RowLayout {
                            Layout.preferredWidth: Theme.tableActionWidth
                            spacing: Theme.space.xs
                            GhostButton {
                                implicitWidth: Theme.compactButtonSize
                                text: "▷"
                                busy: root.pendingJobId === jobRow.jobId
                                enabled: root.pendingJobId.length === 0
                                Accessible.name: qsTr("Run %1 now").arg(jobRow.name)
                                onClicked: root.runNow(jobRow.jobId)
                            }
                            GhostButton {
                                implicitWidth: Theme.compactButtonSize
                                text: "⏻"
                                busy: root.pendingJobId === jobRow.jobId
                                enabled: root.pendingJobId.length === 0
                                Accessible.name: jobRow.enabledState ? qsTr("Disable %1").arg(jobRow.name) : qsTr("Enable %1").arg(jobRow.name)
                                onClicked: root.toggle(jobRow.jobId, jobRow.enabledState)
                            }
                        }
                    }
                    HoverHandler { id: rowTap }
                }
            }
        }
    }

    EmptyState {
        anchors.centerIn: parent
        visible: !cron.loading && jobTable.rows === 0 && root.errorMessage.length === 0
        title: qsTr("No cron jobs")
        detail: qsTr("The live scheduler did not report any jobs.")
        actionText: qsTr("Refresh")
        onActionTriggered: root.refreshView()
    }
    LoadingState { anchors.fill: parent; visible: cron.loading && jobTable.rows === 0 }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0 && jobTable.rows === 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
