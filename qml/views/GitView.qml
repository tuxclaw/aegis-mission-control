pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Aegis 1.0
import "../theme"
import "../components"

Item {
    id: root

    signal openSettingsRequested

    property string selectedPath: ""
    property int selectedIndexState: GitFileState.Untracked
    property int selectedWorktreeState: GitFileState.Untracked
    property string errorMessage: ""
    property bool retryable: false
    property string pendingAction: ""

    function refreshView() {
        errorMessage = "";
        git.refresh();
    }

    function stateGlyph(state) {
        if (state === GitFileState.New)
            return "A";
        if (state === GitFileState.Modified)
            return "M";
        if (state === GitFileState.Deleted)
            return "D";
        if (state === GitFileState.Renamed)
            return "R";
        if (state === GitFileState.Conflicted)
            return "!";
        return "?";
    }

    function selectFile(path, indexState, worktreeState) {
        selectedPath = path;
        selectedIndexState = indexState;
        selectedWorktreeState = worktreeState;
    }

    function requestAction(actionName) {
        if (actionName === "commit" && commitField.text.trim().length === 0) {
            ToastHost.warn(qsTr("Enter a commit message first."));
            commitField.forceActiveFocus();
            return;
        }
        pendingAction = actionName;
        confirmDialog.action = actionName === "commit" ? qsTr("Commit staged paths?") : actionName === "pull" ? qsTr("Pull from %1?").arg(settings.gitRemoteName) : qsTr("Push to %1?").arg(settings.gitRemoteName);
        confirmDialog.detail = actionName === "commit" ? qsTr("Only the explicitly staged paths will be committed on %1.").arg(git.branch) : qsTr("Remote operation for branch %1. Pull is fast-forward only.").arg(git.branch);
        confirmDialog.destructive = actionName !== "pull";
        confirmDialog.open();
    }

    Connections {
        target: git
        function onErrorRaised(message, canRetry) {
            root.errorMessage = message.toLowerCase().indexOf("fast-forward") >= 0 ? qsTr("Pull would not fast-forward — resolve manually") : message;
            root.retryable = canRetry;
            ToastHost.error(root.errorMessage);
        }
        function onToast(message, level) {
            ToastHost.show(message, level);
        }
    }

    EmptyState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: !git.repoConfigured
        title: qsTr("No repository configured")
        detail: qsTr("Choose a Git worktree in Settings before using repository actions.")
        actionText: qsTr("Open Settings")
        onActionTriggered: root.openSettingsRequested()
    }

    ColumnLayout {
        anchors.fill: parent
        visible: git.repoConfigured
        spacing: Theme.viewGutter

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space.md
            Text {
                text: qsTr("BRANCH")
                color: Theme.textMuted
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
            }
            Text {
                text: git.branch
                color: Theme.accent
                font.family: Typography.data.family
                font.pixelSize: Typography.data.pixelSize
                font.weight: Typography.data.weight
            }
            Text {
                text: qsTr("↑%1  ↓%2").arg(git.ahead).arg(git.behind)
                color: git.behind > 0 ? Theme.warn : Theme.textSecondary
                font.family: Typography.dataSmall.family
                font.pixelSize: Typography.dataSmall.pixelSize
                font.weight: Typography.dataSmall.weight
            }
            Item {
                Layout.fillWidth: true
            }
            Rectangle {
                Layout.preferredWidth: Theme.statusDotSize
                Layout.preferredHeight: Theme.statusDotSize
                radius: width / 2
                color: git.clean ? Theme.ok : Theme.warn
            }
            Text {
                text: git.clean ? qsTr("Clean") : qsTr("Changes")
                color: git.clean ? Theme.ok : Theme.warn
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.viewGutter

            GlassCard {
                title: qsTr("CHANGES")
                enterDelay: 0
                Layout.preferredWidth: Math.max(Theme.splitPaneMinimumWidth, root.width * 0.42)
                Layout.fillHeight: true

                Flickable {
                    width: parent.width
                    height: parent.height
                    contentWidth: width
                    contentHeight: changeGroups.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Column {
                        id: changeGroups
                        width: parent.width
                        spacing: Theme.space.sm

                        Repeater {
                            model: [qsTr("STAGED"), qsTr("UNSTAGED"), qsTr("UNTRACKED")]

                            Column {
                                id: groupColumn
                                required property string modelData
                                required property int index
                                width: changeGroups.width
                                spacing: Theme.space.xs

                                Text {
                                    width: parent.width
                                    text: groupColumn.modelData
                                    color: Theme.textMuted
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                    font.letterSpacing: Typography.caption.letterSpacing
                                }
                                ListView {
                                    id: groupList
                                    width: parent.width
                                    height: contentHeight
                                    interactive: false
                                    model: git.files
                                    boundsBehavior: Flickable.StopAtBounds
                                    clip: true

                                    delegate: Rectangle {
                                        id: fileRow
                                        required property var model
                                        property string filePath: model.path
                                        required property int indexState
                                        required property int worktreeState
                                        required property bool staged
                                        readonly property bool belongs: groupColumn.index === 0 ? staged : groupColumn.index === 2 ? (!staged && worktreeState === GitFileState.Untracked) : (!staged && worktreeState !== GitFileState.Untracked)
                                        width: groupList.width
                                        height: belongs ? Theme.tableRowHeight : 0
                                        visible: belongs
                                        radius: Theme.radiusControl
                                        color: root.selectedPath === filePath ? Theme.accentSoft : fileHover.hovered ? Theme.skeletonBase : Theme.transparent
                                        Accessible.name: qsTr("Select changed path %1").arg(fileRow.filePath)
                                        Accessible.role: Accessible.Button

                                        RowLayout {
                                            anchors {
                                                fill: parent
                                                leftMargin: Theme.space.sm
                                                rightMargin: Theme.space.sm
                                            }
                                            spacing: Theme.space.sm
                                            CheckBox {
                                                checked: fileRow.staged
                                                Accessible.name: fileRow.staged ? qsTr("Unstage %1").arg(fileRow.filePath) : qsTr("Stage %1").arg(fileRow.filePath)
                                                onClicked: fileRow.staged ? git.unstagePath(fileRow.filePath) : git.stagePath(fileRow.filePath)
                                            }
                                            Text {
                                                text: root.stateGlyph(fileRow.staged ? fileRow.indexState : fileRow.worktreeState)
                                                color: fileRow.worktreeState === GitFileState.Conflicted ? Theme.alert : fileRow.staged ? Theme.ok : Theme.warn
                                                font.family: Typography.dataSmall.family
                                                font.pixelSize: Typography.dataSmall.pixelSize
                                                font.weight: Typography.dataSmall.weight
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                text: fileRow.filePath
                                                color: Theme.textPrimary
                                                elide: Text.ElideMiddle
                                                font.family: Typography.dataSmall.family
                                                font.pixelSize: Typography.dataSmall.pixelSize
                                                font.weight: Typography.dataSmall.weight
                                            }
                                            GhostButton {
                                                visible: !fileRow.staged
                                                implicitWidth: Theme.compactButtonSize
                                                text: "[+]"
                                                Accessible.name: qsTr("Stage %1").arg(fileRow.filePath)
                                                onClicked: git.stagePath(fileRow.filePath)
                                            }
                                        }
                                        HoverHandler {
                                            id: fileHover
                                            cursorShape: Qt.PointingHandCursor
                                        }
                                        TapHandler {
                                            onTapped: root.selectFile(fileRow.filePath, fileRow.indexState, fileRow.worktreeState)
                                        }
                                    }
                                }
                            }
                        }

                        EmptyState {
                            width: parent.width
                            visible: git.clean
                            title: qsTr("Working tree clean")
                            detail: qsTr("There are no staged or unstaged paths.")
                        }
                    }
                }
            }

            GlassCard {
                title: qsTr("DETAIL / COMMIT")
                enterDelay: Motion.stagger
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    spacing: Theme.space.lg

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: Theme.textAreaMinimumHeight
                        radius: Theme.radiusControl
                        color: Theme.bgElevated
                        border.width: Theme.borderWidth
                        border.color: Theme.divider
                        ColumnLayout {
                            anchors {
                                fill: parent
                                margins: Theme.space.lg
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.selectedPath.length > 0 ? root.selectedPath : qsTr("Select a changed path")
                                color: Theme.textPrimary
                                elide: Text.ElideMiddle
                                font.family: Typography.data.family
                                font.pixelSize: Typography.data.pixelSize
                                font.weight: Typography.data.weight
                            }
                            Text {
                                visible: root.selectedPath.length > 0
                                text: qsTr("Index: %1 · Worktree: %2").arg(root.stateGlyph(root.selectedIndexState)).arg(root.stateGlyph(root.selectedWorktreeState))
                                color: Theme.textSecondary
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }
                    }

                    TextField {
                        id: commitField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Commit message")
                        Accessible.name: qsTr("Git commit message")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PrimaryButton {
                            text: qsTr("Commit")
                            busy: git.busy && root.pendingAction === "commit"
                            enabled: !git.busy && !git.clean
                            Accessible.name: qsTr("Commit staged paths")
                            onClicked: root.requestAction("commit")
                        }
                        SecondaryButton {
                            text: qsTr("Pull")
                            busy: git.busy && root.pendingAction === "pull"
                            enabled: !git.busy
                            Accessible.name: qsTr("Pull current branch")
                            onClicked: root.requestAction("pull")
                        }
                        SecondaryButton {
                            text: qsTr("Push")
                            busy: git.busy && root.pendingAction === "push"
                            enabled: !git.busy
                            Accessible.name: qsTr("Push current branch")
                            onClicked: root.requestAction("push")
                        }
                    }
                    Item {
                        Layout.fillHeight: true
                    }

                    ErrorState {
                        Layout.fillWidth: true
                        visible: root.errorMessage.length > 0
                        userMessage: root.errorMessage
                        retryable: root.retryable
                        onRetryRequested: root.refreshView()
                    }
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: git.busy && root.pendingAction.length === 0
    }

    ConfirmDialog {
        id: confirmDialog
        onAccepted: {
            if (root.pendingAction === "commit")
                git.commit(commitField.text.trim());
            else if (root.pendingAction === "pull")
                git.pull();
            else if (root.pendingAction === "push")
                git.push();
        }
        onRejected: root.pendingAction = ""
    }
}
