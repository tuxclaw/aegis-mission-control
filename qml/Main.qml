pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Aegis 1.0
import "theme"
import "components"
import "views"

ApplicationWindow {
    id: root

    width: Theme.defaultWindowWidth
    height: Theme.defaultWindowHeight
    minimumWidth: Theme.minimumWindowWidth
    minimumHeight: Theme.minimumWindowHeight
    visible: true
    color: Theme.bg
    title: qsTr("AEGIS Mission Control")

    readonly property bool sidebarExpanded: !manuallyCollapsed && width >= Theme.sidebarAutoCollapseWidth
    property bool manuallyCollapsed: false

    function selectView(index) {
        app.activeView = Math.max(0, Math.min(allViewsModel.count - 1, index));
    }

    function refreshCurrentView() {
        if (app.activeView === 0)
            dashboardView.refreshView();
        else if (app.activeView === 1)
            agentRosterView.refreshView();
        else if (app.activeView === 2)
            calendarView.refreshView();
        else if (app.activeView === 3)
            cronView.refreshView();
        else if (app.activeView === 4)
            memoryView.refreshView();
        else if (app.activeView === 5)
            modelView.refreshView();
        else if (app.activeView === 6)
            packagesView.refreshView();
        else if (app.activeView === 7)
            gitView.refreshView();
        else if (app.activeView === 8)
            creativeView.refreshView();
        else
            settingsView.refreshView();
    }

    function connectionKey() {
        return app.connectionState === ConnectionState.Connected ? "connected" : app.connectionState === ConnectionState.Connecting ? "connecting" : "disconnected";
    }

    function connectionLabel() {
        return app.connectionState === ConnectionState.Connected ? qsTr("Connected") : app.connectionState === ConnectionState.Connecting ? qsTr("Connecting") : app.connectionState === ConnectionState.Error ? qsTr("Unavailable") : qsTr("Disconnected");
    }

    ListModel {
        id: navigationModel
        ListElement {
            name: "Dashboard"
            iconPath: "qrc:/icons/dashboard.svg"
        }
        ListElement {
            name: "Agents"
            iconPath: "qrc:/icons/agents.svg"
        }
        ListElement {
            name: "Calendar"
            iconPath: "qrc:/icons/calendar.svg"
        }
        ListElement {
            name: "Cron"
            iconPath: "qrc:/icons/clock.svg"
        }
        ListElement {
            name: "Memory"
            iconPath: "qrc:/icons/memory.svg"
        }
        ListElement {
            name: "Models"
            iconPath: "qrc:/icons/models.svg"
        }
        ListElement {
            name: "Packages"
            iconPath: "qrc:/icons/package.svg"
        }
        ListElement {
            name: "Git"
            iconPath: "qrc:/icons/git-branch.svg"
        }
        ListElement {
            name: "Creative"
            iconPath: "qrc:/icons/creative.svg"
        }
    }

    ListModel {
        id: allViewsModel
        ListElement {
            name: "Dashboard"
        }
        ListElement {
            name: "Agents"
        }
        ListElement {
            name: "Calendar"
        }
        ListElement {
            name: "Cron"
        }
        ListElement {
            name: "Memory"
        }
        ListElement {
            name: "Models"
        }
        ListElement {
            name: "Packages"
        }
        ListElement {
            name: "Git"
        }
        ListElement {
            name: "Creative"
        }
        ListElement {
            name: "Settings"
        }
    }

    RadarGrid {
        anchors.fill: parent
    }

    RowLayout {
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            bottom: statusBar.top
        }
        spacing: 0

        Rectangle {
            id: sidebar
            property real animatedWidth: root.sidebarExpanded ? Theme.sidebarExpandedWidth : Theme.sidebarCollapsedWidth
            Layout.fillHeight: true
            Layout.preferredWidth: animatedWidth
            color: Theme.bgElevated
            border.width: Theme.borderWidth
            border.color: Theme.divider
            clip: true

            Behavior on animatedWidth {
                NumberAnimation {
                    duration: Motion.sidebarCollapse
                    easing.type: Motion.sidebarCollapseEasing
                }
            }

            Item {
                id: wordmark
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }
                height: Theme.topBarHeight + Theme.space.xl
                Accessible.name: qsTr("AEGIS Mission Control home")
                Accessible.role: Accessible.Button

                Row {
                    anchors {
                        left: parent.left
                        leftMargin: Theme.space.lg
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: Theme.space.md
                    Image {
                        width: Theme.wordmarkIconSize
                        height: width
                        source: "qrc:/icons/aegis.svg"
                        sourceSize.width: width
                        sourceSize.height: height
                    }
                    Column {
                        visible: root.sidebarExpanded
                        spacing: Theme.space.xs
                        Text {
                            text: qsTr("AEGIS")
                            color: Theme.accent
                            font.family: Typography.heading.family
                            font.pixelSize: Typography.heading.pixelSize
                            font.weight: Font.Bold
                            font.letterSpacing: Theme.wordmarkTracking
                        }
                        Text {
                            text: qsTr("MISSION CONTROL")
                            color: Theme.textMuted
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                            font.letterSpacing: Theme.eyebrowTracking
                        }
                    }
                }
                TapHandler {
                    onTapped: root.selectView(0)
                }
            }

            Column {
                id: navigationColumn
                anchors {
                    left: parent.left
                    right: parent.right
                    top: wordmark.bottom
                    leftMargin: Theme.space.sm
                    rightMargin: Theme.space.sm
                }
                spacing: Theme.space.xs

                Repeater {
                    model: navigationModel
                    SidebarItem {
                        required property string name
                        required property string iconPath
                        required property int index
                        width: navigationColumn.width
                        label: name
                        iconSource: iconPath
                        showLabel: root.sidebarExpanded
                        active: app.activeView === index
                        badgeText: index === 1 && agents.activeCount > 0 ? String(agents.activeCount) : index === 7 && !git.clean ? "•" : ""
                        badgeColor: index === 7 ? Theme.warn : Theme.accent
                        onClicked: root.selectView(index)
                    }
                }
            }

            Column {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                    margins: Theme.space.sm
                }
                spacing: Theme.space.xs
                Rectangle {
                    width: parent.width
                    height: Theme.borderWidth
                    color: Theme.divider
                }
                SidebarItem {
                    width: parent.width
                    label: qsTr("Settings")
                    iconSource: "qrc:/icons/settings.svg"
                    showLabel: root.sidebarExpanded
                    active: app.activeView === 9
                    onClicked: root.selectView(9)
                }
                GhostButton {
                    width: parent.width
                    implicitWidth: parent.width
                    text: root.sidebarExpanded ? "‹" : "›"
                    Accessible.name: root.sidebarExpanded ? qsTr("Collapse sidebar") : qsTr("Expand sidebar")
                    onClicked: root.manuallyCollapsed = !root.manuallyCollapsed
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.topBarHeight
                color: Theme.bgElevated
                border.width: Theme.borderWidth
                border.color: Theme.divider

                RowLayout {
                    anchors {
                        fill: parent
                        leftMargin: Theme.space.xl
                        rightMargin: Theme.space.lg
                    }
                    spacing: Theme.space.md
                    Text {
                        Layout.fillWidth: true
                        text: allViewsModel.get(app.activeView).name.toUpperCase()
                        color: Theme.textPrimary
                        elide: Text.ElideRight
                        font.family: Typography.title.family
                        font.pixelSize: Typography.title.pixelSize
                        font.weight: Typography.title.weight
                        font.letterSpacing: Typography.title.letterSpacing
                    }
                    GhostButton {
                        implicitWidth: Theme.compactButtonSize
                        text: "↻"
                        Accessible.name: qsTr("Refresh %1").arg(allViewsModel.get(app.activeView).name)
                        onClicked: root.refreshCurrentView()
                    }
                    GhostButton {
                        visible: app.activeView === 2
                        implicitWidth: Theme.compactButtonSize
                        text: "+"
                        Accessible.name: qsTr("Create calendar event")
                        onClicked: calendarView.addItem()
                    }
                }
            }

            StackLayout {
                id: viewStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.margins: Theme.viewPadding
                currentIndex: app.activeView

                DashboardView {
                    id: dashboardView
                    onOpenAgentsRequested: root.selectView(1)
                }
                AgentRosterView {
                    id: agentRosterView
                }
                CalendarView {
                    id: calendarView
                }
                CronView {
                    id: cronView
                }
                MemoryView {
                    id: memoryView
                }
                ModelView {
                    id: modelView
                }
                PackagesView {
                    id: packagesView
                }
                GitView {
                    id: gitView
                    onOpenSettingsRequested: root.selectView(9)
                }
                CreativeView {
                    id: creativeView
                }
                SettingsView {
                    id: settingsView
                }
            }
        }
    }

    StatusBar {
        id: statusBar
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        connectionState: root.connectionKey()
        connectionLabel: root.connectionLabel()
        activeAgents: agents.activeCount
        totalAgents: agents.totalCount
        cpuPercent: vitals.cpuPct
        memoryPercent: vitals.memPct
        lastSync: app.lastSyncTime ? Qt.formatTime(app.lastSyncTime, "HH:mm:ss") : qsTr("Never")
        syncSerial: app.lastSyncTime ? new Date(app.lastSyncTime).getTime() : 0
    }

    Component.onCompleted: {
        Motion.reduceMotion = settings.reduceMotion;
        ToastHost.host = root.contentItem;
    }
}
