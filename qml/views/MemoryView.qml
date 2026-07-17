pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root

    property string errorMessage: ""
    property bool retryable: false
    property bool busy: false
    property string selectedPath: ""
    property string selectedName: ""
    property real selectedSize: 0
    readonly property int previewByteCap: 5 * 1024 * 1024
    readonly property bool previewTooLarge: selectedSize > previewByteCap || memory.currentContent.length > previewByteCap
    readonly property var contentLines: previewTooLarge ? [] : memory.currentContent.split(/\r?\n/)

    function refreshView() {
        errorMessage = "";
        busy = true;
        memory.refresh();
    }

    function displayLine(line) {
        return line.replace(/^#{1,6}\s*/, "").replace(/\*\*(.*?)\*\*/g, "$1");
    }

    function isHeading(line) {
        return /^#{1,6}\s/.test(line);
    }

    function isList(line) {
        return /^\s*[-*+]\s/.test(line);
    }

    Connections {
        target: memory
        function onCurrentContentChanged() { root.busy = false; }
        function onFilesChanged() { root.busy = false; }
        function onErrorRaised(message, canRetry) {
            root.busy = false;
            root.retryable = canRetry;
            root.errorMessage = message.toLowerCase().indexOf("path") >= 0 || message.toLowerCase().indexOf("access") >= 0 ? qsTr("Access denied") : message;
        }
        function onToast(message, level) { ToastHost.show(message, level); }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.viewGutter

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            Text {
                text: qsTr("ROOT")
                color: Theme.textMuted
                font.family: Typography.caption.family
                font.pixelSize: Typography.caption.pixelSize
                font.weight: Typography.caption.weight
            }
            ComboBox {
                id: rootSelector
                model: memory.rootIds
                currentIndex: Math.max(0, memory.rootIds.indexOf(memory.currentRoot))
                Accessible.name: qsTr("Select memory root")
                onActivated: {
                    root.selectedPath = "";
                    root.selectedName = "";
                    root.busy = true;
                    memory.setRoot(currentText);
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.viewGutter

            GlassCard {
                title: qsTr("FILES")
                Layout.preferredWidth: Math.max(Theme.splitPaneMinimumWidth, root.width / 3)
                Layout.fillHeight: true

                ListView {
                    id: fileList
                    width: parent.width
                    height: parent.height
                    model: memory.files
                    spacing: Theme.space.xs
                    clip: true

                    delegate: Rectangle {
                        id: fileRow
                        required property string name
                        required property string relativePath
                        required property string rootId
                        required property real sizeBytes
                        required property var modifiedAt
                        width: fileList.width
                        height: Theme.tableRowHeight
                        radius: Theme.radiusControl
                        color: root.selectedPath === fileRow.relativePath ? Theme.accentSoft : fileHover.hovered ? Theme.skeletonBase : Theme.transparent
                        border.width: root.selectedPath === fileRow.relativePath ? Theme.borderWidth : 0
                        border.color: Theme.accent

                        RowLayout {
                            anchors { fill: parent; leftMargin: Theme.space.md; rightMargin: Theme.space.md }
                            spacing: Theme.space.sm
                            Text { text: root.selectedPath === fileRow.relativePath ? "▸" : "•"; color: root.selectedPath === fileRow.relativePath ? Theme.accent : Theme.textMuted; font.family: Typography.dataSmall.family; font.pixelSize: Typography.dataSmall.pixelSize; font.weight: Typography.dataSmall.weight }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Theme.space.xs
                                Text { Layout.fillWidth: true; text: fileRow.name; color: Theme.textPrimary; elide: Text.ElideMiddle; font.family: Typography.label.family; font.pixelSize: Typography.label.pixelSize; font.weight: Typography.label.weight }
                                Text { text: qsTr("%1 KB · %2").arg(Math.ceil(fileRow.sizeBytes / 1024)).arg(Qt.formatDateTime(fileRow.modifiedAt, "MMM d  HH:mm")); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                            }
                        }

                        HoverHandler { id: fileHover; cursorShape: Qt.PointingHandCursor }
                        TapHandler {
                            onTapped: {
                                root.selectedPath = fileRow.relativePath;
                                root.selectedName = fileRow.name;
                                root.selectedSize = fileRow.sizeBytes;
                                root.errorMessage = "";
                                if (fileRow.sizeBytes <= root.previewByteCap) {
                                    root.busy = true;
                                    memory.selectFile(fileRow.relativePath);
                                }
                            }
                        }
                        Accessible.name: qsTr("Open %1").arg(fileRow.name)
                        Accessible.role: Accessible.Button
                    }

                    EmptyState {
                        anchors.centerIn: parent
                        visible: fileList.count === 0 && !root.busy && root.errorMessage.length === 0
                        title: qsTr("No memory files")
                        detail: qsTr("This allowlisted root contains no Markdown files.")
                    }
                }
            }

            GlassCard {
                Layout.fillWidth: true
                Layout.fillHeight: true

                ColumnLayout {
                    width: parent.width
                    height: parent.height
                    spacing: Theme.space.md

                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: root.selectedName.length > 0 ? root.selectedName : qsTr("Select a memory file"); color: Theme.textPrimary; elide: Text.ElideMiddle; font.family: Typography.heading.family; font.pixelSize: Typography.heading.pixelSize; font.weight: Typography.heading.weight }
                        Text { visible: root.selectedSize > 0; text: qsTr("%1 KB").arg(Math.ceil(root.selectedSize / 1024)); color: Theme.textMuted; font.family: Typography.dataSmall.family; font.pixelSize: Typography.dataSmall.pixelSize; font.weight: Typography.dataSmall.weight }
                    }

                    ListView {
                        id: contentView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: root.contentLines
                        spacing: Theme.space.xs
                        clip: true

                        delegate: Text {
                            required property string modelData
                            width: contentView.width
                            text: root.displayLine(modelData)
                            color: root.isHeading(modelData) ? Theme.textPrimary : Theme.textSecondary
                            wrapMode: Text.Wrap
                            textFormat: Text.PlainText
                            leftPadding: root.isList(modelData) ? Theme.space.lg : 0
                            font.family: Typography.monoFamily
                            font.pixelSize: root.isHeading(modelData) ? Typography.heading.pixelSize : Typography.body.pixelSize
                            font.weight: root.isHeading(modelData) || /\*\*.*\*\*/.test(modelData) ? Font.DemiBold : Font.Normal
                        }
                    }

                    EmptyState {
                        anchors.centerIn: parent
                        visible: root.selectedPath.length === 0 && !root.busy
                        title: qsTr("Select a memory file")
                        detail: qsTr("Content is shown as inert text with safe Markdown-lite styling.")
                    }
                    ErrorState {
                        anchors.centerIn: parent
                        visible: root.previewTooLarge
                        userMessage: qsTr("File too large to preview")
                    }
                }
            }
        }
    }

    LoadingState { anchors.fill: parent; visible: root.busy }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
