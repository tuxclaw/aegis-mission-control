pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "components"

ApplicationWindow {
    id: root

    width: Theme.defaultWindowWidth
    height: Theme.defaultWindowHeight
    minimumWidth: Theme.minimumWindowWidth
    minimumHeight: Theme.minimumWindowHeight
    visible: true
    color: Theme.bg
    title: qsTr("AEGIS Mission Control")

    property int currentView: 0
    property bool manuallyCollapsed: false
    readonly property bool sidebarExpanded: !manuallyCollapsed && width >= Theme.sidebarAutoCollapseWidth
    property bool reduceMotion: false

    // Integration surface for C++ controllers. Defaults are deliberately honest.
    property string connectionState: "unknown"
    property string connectionLabel: qsTr("Backend unavailable")
    property int activeAgents: 0
    property int totalAgents: 0
    property real cpuPercent: Number.NaN
    property real memoryPercent: Number.NaN
    property string lastSync: qsTr("Never")
    property int syncSerial: 0

    signal refreshRequested(int viewIndex)
    signal addRequested(int viewIndex)

    onReduceMotionChanged: Motion.reduceMotion = reduceMotion

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
                    onTapped: root.currentView = 0
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
                        active: root.currentView === index
                        badgeText: index === 1 && root.activeAgents > 0 ? String(root.activeAgents) : ""
                        badgeColor: Theme.accent
                        onClicked: root.currentView = index
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
                    active: root.currentView === 9
                    onClicked: root.currentView = 9
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
                        text: allViewsModel.get(root.currentView).name.toUpperCase()
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
                        Accessible.name: qsTr("Refresh %1").arg(allViewsModel.get(root.currentView).name)
                        onClicked: root.refreshRequested(root.currentView)
                    }
                    GhostButton {
                        implicitWidth: Theme.compactButtonSize
                        text: "+"
                        Accessible.name: qsTr("Add item to %1").arg(allViewsModel.get(root.currentView).name)
                        onClicked: root.addRequested(root.currentView)
                    }
                }
            }

            StackLayout {
                id: viewStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: root.currentView

                PlaceholderView {
                    viewName: qsTr("Dashboard")
                }
                PlaceholderView {
                    viewName: qsTr("Agents")
                }
                PlaceholderView {
                    viewName: qsTr("Calendar")
                }
                PlaceholderView {
                    viewName: qsTr("Cron")
                }
                PlaceholderView {
                    viewName: qsTr("Memory")
                }
                PlaceholderView {
                    viewName: qsTr("Models")
                }
                PlaceholderView {
                    viewName: qsTr("Packages")
                }
                PlaceholderView {
                    viewName: qsTr("Git")
                }
                PlaceholderView {
                    viewName: qsTr("Creative")
                }
                PlaceholderView {
                    viewName: qsTr("Settings")
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
        connectionState: root.connectionState
        connectionLabel: root.connectionLabel
        activeAgents: root.activeAgents
        totalAgents: root.totalAgents
        cpuPercent: root.cpuPercent
        memoryPercent: root.memoryPercent
        lastSync: root.lastSync
        syncSerial: root.syncSerial
    }

    Component.onCompleted: {
        Motion.reduceMotion = reduceMotion;
        ToastHost.host = root.contentItem;
    }

    component PlaceholderView: Rectangle {
        id: placeholder
        required property string viewName

        color: Theme.transparent

        EmptyState {
            anchors.centerIn: parent
            width: Math.min(parent.width - Theme.space.xxl * 2, Theme.dialogWidth)
            height: implicitHeight
            title: qsTr("%1 view foundation ready").arg(placeholder.viewName)
            detail: qsTr("Live controller-backed content will render here.")
        }
    }
}
