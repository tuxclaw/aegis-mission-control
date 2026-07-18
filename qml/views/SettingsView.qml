pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Aegis 1.0
import "../theme"
import "../components"

Item {
    id: root

    property string errorMessage: ""
    property bool retryable: false
    property bool busy: false
    property var pendingMemoryRoots: ({})

    function refreshView() {
        loadSettings();
    }

    function connectionText() {
        if (app.connectionState === ConnectionState.Connected)
            return qsTr("Connected");
        if (app.connectionState === ConnectionState.Connecting)
            return qsTr("Connecting");
        return qsTr("Disconnected");
    }

    function connectionColor() {
        return app.connectionState === ConnectionState.Connected ? Theme.ok : app.connectionState === ConnectionState.Connecting ? Theme.warn : Theme.alert;
    }

    function loadSettings() {
        gatewayUrlField.text = settings.gatewayBaseUrl;
        gitPathField.text = settings.gitRepoPath;
        remoteField.text = settings.gitRemoteName;
        pullModeBox.currentIndex = Math.max(0, pullModeBox.find(settings.gitPullMode));
        dataRootField.text = settings.dataRoot;
        ollamaBaseUrlField.text = settings.ollamaBaseUrl;
        vitalsIntervalBox.value = settings.vitalsIntervalMs;
        themeBox.currentIndex = Math.max(0, themeBox.find(settings.theme === "light" ? qsTr("Light") : qsTr("Dark")));
        reduceMotionBox.checked = settings.reduceMotion;
        logLevelBox.currentIndex = Math.max(0, logLevelBox.find(settings.logLevel));
        pendingMemoryRoots = Object.assign({}, settings.memoryRoots);
        Motion.reduceMotion = settings.reduceMotion;
    }

    function validGatewayUrl(value) {
        const trimmed = value.trim().toLowerCase();
        const loopback = /^http:\/\/(localhost|127(?:\.\d+){0,3}|\[::1\])(?::\d+)?(?:\/|$)/.test(trimmed);
        return /^https:\/\//.test(trimmed) || loopback;
    }

    function saveSettings() {
        errorMessage = "";
        if (!validGatewayUrl(gatewayUrlField.text)) {
            errorMessage = qsTr("Gateway URLs outside the local machine must use TLS.");
            gatewayUrlField.forceActiveFocus();
            return;
        }
        if (remoteField.text.trim().length === 0) {
            errorMessage = qsTr("Git remote name cannot be empty.");
            remoteField.forceActiveFocus();
            return;
        }
        if (vitalsIntervalBox.value < 250 || vitalsIntervalBox.value > 10000) {
            errorMessage = qsTr("Vitals interval must be between 250 and 10000 milliseconds.");
            return;
        }
        settings.gatewayBaseUrl = gatewayUrlField.text.trim();
        settings.gitRepoPath = gitPathField.text.trim();
        settings.gitRemoteName = remoteField.text.trim();
        settings.gitPullMode = pullModeBox.currentText;
        settings.memoryRoots = pendingMemoryRoots;
        settings.dataRoot = dataRootField.text.trim();
        settings.ollamaBaseUrl = ollamaBaseUrlField.text.trim();
        settings.vitalsIntervalMs = vitalsIntervalBox.value;
        settings.theme = themeBox.currentText.toLowerCase();
        settings.reduceMotion = reduceMotionBox.checked;
        settings.logLevel = logLevelBox.currentText.toLowerCase();
        Motion.reduceMotion = reduceMotionBox.checked;
        busy = true;
        settings.save();
    }

    function updateGatewayToken() {
        if (gatewayTokenField.text.length === 0) {
            errorMessage = qsTr("Enter a gateway token before updating it.");
            return;
        }
        const value = gatewayTokenField.text;
        gatewayTokenField.clear();
        busy = true;
        settings.updateGatewayAccess(value);
    }

    function addMemoryRoot() {
        const rootId = rootIdField.text.trim();
        const rootPath = rootPathField.text.trim();
        if (rootId.length === 0 || rootPath.length === 0) {
            errorMessage = qsTr("Memory root ID and path are required.");
            return;
        }
        const nextRoots = Object.assign({}, pendingMemoryRoots);
        nextRoots[rootId] = rootPath;
        pendingMemoryRoots = nextRoots;
        rootIdField.clear();
        rootPathField.clear();
    }

    Connections {
        target: settings
        function onErrorRaised(message, canRetry) {
            root.busy = false;
            root.errorMessage = message;
            root.retryable = canRetry;
        }
        function onToast(message, level) {
            root.busy = false;
            ToastHost.show(message, level);
            if (level !== "error")
                root.loadSettings();
        }
    }

    component FormLabel: Text {
        Layout.preferredWidth: Theme.settingsLabelWidth
        color: Theme.textSecondary
        font.family: Typography.label.family
        font.pixelSize: Typography.label.pixelSize
        font.weight: Typography.label.weight
        font.letterSpacing: Typography.label.letterSpacing
    }

    component SectionHeading: Text {
        color: Theme.accent
        font.family: Typography.heading.family
        font.pixelSize: Typography.heading.pixelSize
        font.weight: Typography.heading.weight
        font.letterSpacing: Typography.heading.letterSpacing
    }

    FolderDialog {
        id: repoFolderDialog
        title: qsTr("Choose Git worktree")
        onAccepted: gitPathField.text = selectedFolder.toString().replace(/^file:\/\//, "")
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true
        Component.onCompleted: contentItem["boundsBehavior"] = Flickable.StopAtBounds

        ColumnLayout {
            width: parent.width
            spacing: Theme.viewGutter

            GlassCard {
                title: qsTr("GATEWAY")
                enterDelay: 0
                Layout.fillWidth: true

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Base URL")
                        }
                        TextField {
                            id: gatewayUrlField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Gateway base URL")
                            Accessible.name: qsTr("Gateway base URL")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Gateway token")
                        }
                        TextField {
                            id: gatewayTokenField
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: settings.gatewayAccessConfigured ? qsTr("Credential set") : qsTr("Credential not set")
                            Accessible.name: qsTr("New gateway token")
                        }
                        SecondaryButton {
                            text: qsTr("Update token")
                            Accessible.name: qsTr("Update gateway token")
                            onClicked: root.updateGatewayToken()
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Status")
                        }
                        Rectangle {
                            Layout.preferredWidth: Theme.statusDotSize
                            Layout.preferredHeight: Theme.statusDotSize
                            radius: width / 2
                            color: root.connectionColor()
                        }
                        Text {
                            Layout.fillWidth: true
                            text: root.connectionText()
                            color: root.connectionColor()
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                    }
                }
            }

            GlassCard {
                title: qsTr("GIT")
                enterDelay: Motion.stagger
                Layout.fillWidth: true

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Repo path")
                        }
                        TextField {
                            id: gitPathField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Git worktree path")
                            Accessible.name: qsTr("Git repository path")
                        }
                        GhostButton {
                            text: qsTr("Browse")
                            Accessible.name: qsTr("Browse for Git worktree")
                            onClicked: repoFolderDialog.open()
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Remote")
                        }
                        TextField {
                            id: remoteField
                            Layout.fillWidth: true
                            Accessible.name: qsTr("Git remote name")
                        }
                        FormLabel {
                            Layout.preferredWidth: implicitWidth
                            text: qsTr("Pull mode")
                        }
                        ComboBox {
                            id: pullModeBox
                            model: ["ff-only"]
                            Accessible.name: qsTr("Git pull mode")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Credential")
                        }
                        TextField {
                            id: gitCredentialField
                            Layout.fillWidth: true
                            echoMode: TextInput.Password
                            placeholderText: settings.gitAccessConfigured ? qsTr("Credential set") : qsTr("Credential not set")
                            readOnly: true
                            Accessible.name: qsTr("Git credential status")
                        }
                        SecondaryButton {
                            text: qsTr("Update")
                            enabled: false
                            Accessible.name: qsTr("Git credential updates require credential-store support")
                            ToolTip.visible: hovered
                            ToolTip.text: qsTr("Credential updates are unavailable until the controller exposes a secret-store intent.")
                        }
                    }
                    Text {
                        Layout.fillWidth: true
                        text: qsTr("The controller validates that the selected path is a Git worktree when saving.")
                        color: Theme.textMuted
                        wrapMode: Text.Wrap
                        font.family: Typography.caption.family
                        font.pixelSize: Typography.caption.pixelSize
                        font.weight: Typography.caption.weight
                    }
                }
            }

            GlassCard {
                title: qsTr("MEMORY ROOTS")
                enterDelay: 2 * Motion.stagger
                Layout.fillWidth: true

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md
                    Repeater {
                        model: Object.keys(root.pendingMemoryRoots)
                        RowLayout {
                            required property string modelData
                            Layout.fillWidth: true
                            Text {
                                Layout.preferredWidth: Theme.settingsLabelWidth
                                text: modelData
                                color: Theme.accent
                                elide: Text.ElideRight
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: root.pendingMemoryRoots[modelData]
                                color: Theme.textSecondary
                                elide: Text.ElideMiddle
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }
                    }
                    EmptyState {
                        Layout.fillWidth: true
                        visible: Object.keys(root.pendingMemoryRoots).length === 0
                        title: qsTr("No memory roots")
                        detail: qsTr("Add an allowlisted root before using Memory.")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        TextField {
                            id: rootIdField
                            Layout.preferredWidth: Theme.settingsLabelWidth
                            placeholderText: qsTr("Root ID")
                            Accessible.name: qsTr("New memory root ID")
                        }
                        TextField {
                            id: rootPathField
                            Layout.fillWidth: true
                            placeholderText: qsTr("Allowlisted path")
                            Accessible.name: qsTr("New memory root path")
                        }
                        GhostButton {
                            text: qsTr("+ Add")
                            Accessible.name: qsTr("Add memory root")
                            onClicked: root.addMemoryRoot()
                        }
                    }
                }
            }

            GlassCard {
                title: qsTr("SYSTEM & APPEARANCE")
                enterDelay: 3 * Motion.stagger
                Layout.fillWidth: true

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.md
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Data root")
                        }
                        TextField {
                            id: dataRootField
                            Layout.fillWidth: true
                            Accessible.name: qsTr("AEGIS data root")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Ollama URL")
                        }
                        TextField {
                            id: ollamaBaseUrlField
                            Layout.fillWidth: true
                            Accessible.name: qsTr("Ollama base URL")
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Vitals interval")
                        }
                        SpinBox {
                            id: vitalsIntervalBox
                            from: 250
                            to: 10000
                            editable: true
                            Accessible.name: qsTr("Vitals interval milliseconds")
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("ms")
                            color: Theme.textMuted
                            font.family: Typography.dataSmall.family
                            font.pixelSize: Typography.dataSmall.pixelSize
                            font.weight: Typography.dataSmall.weight
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Appearance")
                        }
                        ComboBox {
                            id: themeBox
                            model: [qsTr("Dark"), qsTr("Light")]
                            Accessible.name: qsTr("Application theme")
                        }
                        CheckBox {
                            id: reduceMotionBox
                            text: qsTr("Reduce motion")
                            Accessible.name: qsTr("Reduce interface motion")
                            onToggled: Motion.reduceMotion = checked
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        FormLabel {
                            text: qsTr("Logging")
                        }
                        ComboBox {
                            id: logLevelBox
                            model: ["error", "warn", "info", "debug"]
                            Accessible.name: qsTr("Logging level")
                        }
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                visible: root.errorMessage.length > 0
                text: root.errorMessage
                color: Theme.alert
                wrapMode: Text.Wrap
                font.family: Typography.body.family
                font.pixelSize: Typography.body.pixelSize
                font.weight: Typography.body.weight
            }
            RowLayout {
                Layout.fillWidth: true
                GhostButton {
                    text: qsTr("Reset defaults")
                    destructive: true
                    Accessible.name: qsTr("Reset settings to defaults")
                    onClicked: resetDialog.open()
                }
                Item {
                    Layout.fillWidth: true
                }
                PrimaryButton {
                    text: qsTr("Save Settings")
                    busy: root.busy
                    Accessible.name: qsTr("Save settings")
                    onClicked: root.saveSettings()
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: root.busy
    }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0 && root.retryable
        userMessage: root.errorMessage
        retryable: true
        onRetryRequested: root.loadSettings()
    }
    ConfirmDialog {
        id: resetDialog
        action: qsTr("Reset settings to defaults?")
        detail: qsTr("Configured values will return to validated defaults. Secrets remain in the credential store.")
        destructive: true
        onAccepted: {
            root.busy = true;
            settings.resetDefaults();
        }
    }

    Component.onCompleted: loadSettings()
}
