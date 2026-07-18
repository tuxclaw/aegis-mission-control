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

    readonly property string errorMessage: String(Usage.errorMessage || "")
    readonly property int providerCount: Usage.providers ? Usage.providers.length : 0

    function refreshView() {
        Usage.refresh();
    }

    function clampFraction(value) {
        const fraction = Number(value);
        return Number.isFinite(fraction) ? Math.max(0, Math.min(1, fraction)) : 0;
    }

    function compactNumber(value) {
        const amount = Number(value);
        if (!Number.isFinite(amount))
            return "0";
        if (Math.abs(amount) >= 1000000000)
            return (amount / 1000000000).toFixed(amount >= 10000000000 ? 0 : 1) + "B";
        if (Math.abs(amount) >= 1000000)
            return (amount / 1000000).toFixed(amount >= 10000000 ? 0 : 1) + "M";
        if (Math.abs(amount) >= 1000)
            return (amount / 1000).toFixed(amount >= 10000 ? 0 : 1) + "k";
        return amount.toFixed(0);
    }

    function formatCurrency(amount, currency) {
        const value = Number(amount);
        if (!Number.isFinite(value))
            return "";
        const code = String(currency || "").toUpperCase();
        return (code === "" || code === "USD" ? "$" : code + " ") + value.toFixed(2);
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        Component.onCompleted: contentItem["boundsBehavior"] = Flickable.StopAtBounds

        ColumnLayout {
            width: parent.width
            spacing: Theme.viewGutter

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.space.md

                Text {
                    Layout.fillWidth: true
                    text: qsTr("USAGE")
                    color: Theme.textPrimary
                    font.family: Typography.title.family
                    font.pixelSize: Typography.title.pixelSize
                    font.weight: Typography.title.weight
                    font.letterSpacing: Typography.title.letterSpacing
                }

                SecondaryButton {
                    text: qsTr("Refresh")
                    busy: Usage.loading
                    Accessible.name: qsTr("Refresh provider usage")
                    onClicked: root.refreshView()
                }
            }

            ErrorState {
                Layout.fillWidth: true
                visible: root.errorMessage.length > 0
                userMessage: root.errorMessage
                retryable: true
                onRetryRequested: root.refreshView()
            }

            GridLayout {
                id: providerGrid
                Layout.fillWidth: true
                visible: root.errorMessage.length === 0 && root.providerCount > 0
                columns: width >= 1080 ? 3 : width >= 680 ? 2 : 1
                columnSpacing: Theme.viewGutter
                rowSpacing: Theme.viewGutter

                Repeater {
                    model: Usage.providers

                    GlassCard {
                        id: providerCard

                        required property var modelData
                        required property int index

                        readonly property string providerName: String(modelData.displayName || modelData.id || qsTr("Provider"))
                        readonly property string plan: String(modelData.planName || "")
                        readonly property bool providerEnabled: Boolean(modelData.enabled)
                        readonly property bool providerFetched: Boolean(modelData.fetched)
                        readonly property bool providerHasError: Boolean(modelData.hasError)
                        readonly property string providerError: String(modelData.errorMsg || qsTr("Usage data is unavailable."))
                        readonly property real usedFraction: root.clampFraction(modelData.primaryFraction)
                        readonly property var usageWindows: modelData.windows || []
                        readonly property var primaryWindow: usageWindows.length > 0 ? usageWindows[0] : null
                        readonly property real tokensUsed: primaryWindow ? Number(primaryWindow.tokensUsed || 0) : 0
                        readonly property real tokensLimit: primaryWindow ? Number(primaryWindow.tokensLimit || 0) : 0
                        readonly property string currency: String(modelData.balanceCurrency || "")
                        readonly property real balance: Number(modelData.balanceTotal || 0)
                        readonly property bool hasBalance: currency.length > 0 || Number(modelData.balanceCash || 0) !== 0 || Number(modelData.balanceGift || 0) !== 0 || balance !== 0
                        readonly property var fetchedTime: modelData.fetchedAt ? new Date(modelData.fetchedAt) : null
                        readonly property bool hasFetchedTime: fetchedTime !== null && Number.isFinite(fetchedTime.getTime())
                        readonly property color usageColor: usedFraction > 0.8 ? Theme.alert : usedFraction >= 0.5 ? Theme.warn : Theme.ok

                        backdrop: root.backdrop
                        liveTracking: true
                        enterDelay: index * Motion.stagger
                        Layout.fillWidth: true
                        Layout.minimumWidth: Theme.minimumCardWidth
                        Layout.minimumHeight: Theme.contentMinimumHeight - Theme.space.xl

                        RowLayout {
                            width: parent.width
                            spacing: Theme.space.sm

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs

                                Text {
                                    Layout.fillWidth: true
                                    text: providerCard.providerName
                                    textFormat: Text.PlainText
                                    color: Theme.textPrimary
                                    elide: Text.ElideRight
                                    font.family: Typography.heading.family
                                    font.pixelSize: Typography.heading.pixelSize
                                    font.weight: Typography.heading.weight
                                    font.letterSpacing: Typography.heading.letterSpacing
                                }

                                Text {
                                    Layout.fillWidth: true
                                    visible: providerCard.plan.length > 0
                                    text: providerCard.plan
                                    textFormat: Text.PlainText
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }

                            Rectangle {
                                Layout.alignment: Qt.AlignTop
                                Layout.preferredWidth: enabledLabel.implicitWidth + Theme.space.md
                                Layout.preferredHeight: Theme.chipHeight
                                radius: Theme.radiusPill
                                color: providerCard.providerEnabled ? Theme.okSoft : Theme.skeletonBase

                                Text {
                                    id: enabledLabel
                                    anchors.centerIn: parent
                                    text: providerCard.providerEnabled ? qsTr("ENABLED") : qsTr("DISABLED")
                                    color: providerCard.providerEnabled ? Theme.ok : Theme.textMuted
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                            }
                        }

                        Column {
                            width: parent.width
                            visible: providerCard.providerFetched && !providerCard.providerHasError
                            spacing: Theme.space.sm

                            RowLayout {
                                width: parent.width

                                Text {
                                    Layout.fillWidth: true
                                    text: providerCard.tokensLimit > 0 ? qsTr("%1 / %2 tokens").arg(root.compactNumber(providerCard.tokensUsed)).arg(root.compactNumber(providerCard.tokensLimit)) : qsTr("%1% used").arg(Math.round(providerCard.usedFraction * 100))
                                    color: Theme.textSecondary
                                    elide: Text.ElideRight
                                    font.family: Typography.dataSmall.family
                                    font.pixelSize: Typography.dataSmall.pixelSize
                                    font.weight: Typography.dataSmall.weight
                                }

                                Text {
                                    text: qsTr("%1%").arg(Math.round(providerCard.usedFraction * 100))
                                    color: providerCard.usageColor
                                    font.family: Typography.data.family
                                    font.pixelSize: Typography.data.pixelSize
                                    font.weight: Typography.data.weight
                                }
                            }

                            Rectangle {
                                width: parent.width
                                height: Theme.usageBarHeight
                                radius: Theme.radiusPill
                                color: Theme.skeletonBase

                                Rectangle {
                                    width: parent.width * providerCard.usedFraction
                                    height: parent.height
                                    radius: parent.radius
                                    color: providerCard.usageColor

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
                            width: parent.width
                            visible: providerCard.hasBalance && !providerCard.providerHasError
                            text: qsTr("%1 remaining").arg(root.formatCurrency(providerCard.balance, providerCard.currency))
                            color: providerCard.balance > 0 ? Theme.ok : Theme.warn
                            elide: Text.ElideRight
                            font.family: Typography.data.family
                            font.pixelSize: Typography.data.pixelSize
                            font.weight: Typography.data.weight
                        }

                        ErrorState {
                            width: parent.width
                            visible: providerCard.providerHasError
                            userMessage: providerCard.providerError
                            retryable: false
                        }

                        Item {
                            width: parent.width
                            height: Theme.borderWidth
                            visible: providerCard.providerFetched || providerCard.providerHasError || providerCard.hasBalance

                            Rectangle {
                                anchors.fill: parent
                                color: Theme.divider
                            }
                        }

                        Text {
                            width: parent.width
                            text: providerCard.providerFetched ? providerCard.hasFetchedTime ? qsTr("Updated: %1").arg(Qt.formatTime(providerCard.fetchedTime, "HH:mm")) : qsTr("Updated: --:--") : qsTr("Not fetched")
                            color: Theme.textMuted
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                        }
                    }
                }
            }

            LoadingState {
                Layout.fillWidth: true
                Layout.minimumHeight: Theme.contentMinimumHeight
                visible: root.errorMessage.length === 0 && root.providerCount === 0 && Usage.loading
            }

            EmptyState {
                Layout.fillWidth: true
                Layout.minimumHeight: Theme.contentMinimumHeight
                visible: root.errorMessage.length === 0 && root.providerCount === 0 && !Usage.loading
                icon: "qrc:/icons/usage.svg"
                title: qsTr("No usage providers")
                detail: qsTr("Provider quotas will appear here when usage tracking is configured.")
                actionText: qsTr("Refresh")
                onActionTriggered: root.refreshView()
            }
        }
    }
}
