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
    property int outputBaseLength: 0
    readonly property string visibleOutput: creative.output.slice(Math.min(outputBaseLength, creative.output.length))

    function refreshView() {
        errorMessage = "";
        models.refresh();
    }

    function generate() {
        const prompt = promptField.text.trim();
        if (prompt.length < 1 || prompt.length > 8000) {
            errorMessage = qsTr("Prompt must be between 1 and 8000 characters.");
            return;
        }
        if (modelSelector.currentValue === undefined || String(modelSelector.currentValue).length === 0) {
            errorMessage = qsTr("Choose a model from the live list.");
            return;
        }
        errorMessage = "";
        outputBaseLength = 0;
        creative.generate(backendSelector.currentIndex === 0 ? CreativeBackend.Ollama : CreativeBackend.GatewayCreative,
                          String(modelSelector.currentValue), prompt, temperatureSlider.value, tokenBox.value);
    }

    Connections {
        target: creative
        function onOutputChanged() { outputText.cursorPosition = outputText.length; }
        function onErrorRaised(message, canRetry) {
            root.errorMessage = message;
            root.retryable = canRetry;
            ToastHost.error(message);
        }
        function onToast(message, level) { ToastHost.show(message, level); }
    }

    RowLayout {
        anchors.fill: parent
        spacing: Theme.viewGutter

        GlassCard {
            title: qsTr("CONFIG")
            Layout.preferredWidth: Math.max(Theme.splitPaneMinimumWidth, root.width * 0.36)
            Layout.fillHeight: true

            ScrollView {
                width: parent.width
                height: parent.height
                contentWidth: availableWidth

                ColumnLayout {
                    width: parent.width
                    spacing: Theme.space.lg

                    Text { text: qsTr("BACKEND"); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                    ComboBox {
                        id: backendSelector
                        Layout.fillWidth: true
                        model: [qsTr("Ollama"), qsTr("Gateway")]
                        Accessible.name: qsTr("Creative backend")
                    }
                    Text { text: qsTr("MODEL"); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                    ComboBox {
                        id: modelSelector
                        Layout.fillWidth: true
                        model: models.models
                        textRole: "label"
                        valueRole: "id"
                        Accessible.name: qsTr("Creative model")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { text: qsTr("TEMPERATURE"); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                        Item { Layout.fillWidth: true }
                        Text { text: temperatureSlider.value.toFixed(1); color: Theme.accent; font.family: Typography.dataSmall.family; font.pixelSize: Typography.dataSmall.pixelSize; font.weight: Typography.dataSmall.weight }
                    }
                    Slider {
                        id: temperatureSlider
                        Layout.fillWidth: true
                        from: 0
                        to: 2
                        stepSize: 0.1
                        value: 0.7
                        Accessible.name: qsTr("Generation temperature")
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text { Layout.fillWidth: true; text: qsTr("MAX TOKENS"); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                        SpinBox {
                            id: tokenBox
                            from: 1
                            to: 8192
                            value: 1024
                            editable: true
                            Accessible.name: qsTr("Maximum output tokens")
                        }
                    }
                    Text { text: qsTr("PROMPT"); color: Theme.textMuted; font.family: Typography.caption.family; font.pixelSize: Typography.caption.pixelSize; font.weight: Typography.caption.weight }
                    TextArea {
                        id: promptField
                        Layout.fillWidth: true
                        Layout.minimumHeight: Theme.textAreaMinimumHeight * 2
                        placeholderText: qsTr("What should AEGIS create?")
                        wrapMode: TextArea.Wrap
                        Accessible.name: qsTr("Creative prompt")
                        onTextChanged: if (length > 8000) remove(8000, length)
                    }
                    Text {
                        visible: root.errorMessage.length > 0
                        Layout.fillWidth: true
                        text: root.errorMessage
                        color: Theme.alert
                        wrapMode: Text.Wrap
                        font.family: Typography.body.family
                        font.pixelSize: Typography.body.pixelSize
                        font.weight: Typography.body.weight
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        PrimaryButton { text: qsTr("Generate"); busy: creative.busy; enabled: !creative.busy; Accessible.name: qsTr("Generate creative output"); onClicked: root.generate() }
                        SecondaryButton { text: qsTr("Cancel"); destructive: true; enabled: creative.busy; Accessible.name: qsTr("Cancel generation"); onClicked: creative.cancel() }
                    }
                }
            }
        }

        GlassCard {
            title: qsTr("OUTPUT")
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                height: parent.height
                spacing: Theme.space.md

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    TextArea {
                        id: outputText
                        width: parent.width
                        readOnly: true
                        selectByMouse: true
                        text: root.visibleOutput
                        textFormat: TextEdit.PlainText
                        wrapMode: TextArea.Wrap
                        color: Theme.textPrimary
                        font.family: Typography.monoFamily
                        font.pixelSize: Typography.body.pixelSize
                        font.weight: Font.Normal
                        Accessible.name: qsTr("Generated output")
                    }
                }

                EmptyState {
                    anchors.centerIn: parent
                    visible: root.visibleOutput.length === 0 && !creative.busy && root.errorMessage.length === 0
                    title: qsTr("No output yet")
                    detail: qsTr("Choose a live model and generate from a prompt.")
                }

                RowLayout {
                    Layout.fillWidth: true
                    Text { text: qsTr("finish: %1").arg(creative.finishReason.length > 0 ? creative.finishReason : "—"); color: Theme.textMuted; font.family: Typography.dataSmall.family; font.pixelSize: Typography.dataSmall.pixelSize; font.weight: Typography.dataSmall.weight }
                    Text { text: qsTr("out: %1 B").arg(creative.outputBytes); color: Theme.textMuted; font.family: Typography.dataSmall.family; font.pixelSize: Typography.dataSmall.pixelSize; font.weight: Typography.dataSmall.weight }
                    Item { Layout.fillWidth: true }
                    GhostButton {
                        text: qsTr("Copy")
                        enabled: root.visibleOutput.length > 0
                        Accessible.name: qsTr("Copy generated output")
                        onClicked: {
                            outputText.selectAll();
                            outputText.copy();
                            outputText.deselect();
                            ToastHost.success(qsTr("Output copied."));
                        }
                    }
                    GhostButton {
                        text: qsTr("Clear")
                        enabled: root.visibleOutput.length > 0 && !creative.busy
                        Accessible.name: qsTr("Clear generated output")
                        onClicked: root.outputBaseLength = creative.output.length
                    }
                }
            }
        }
    }

    LoadingState { anchors.fill: parent; visible: models.loading && modelSelector.count === 0 }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0 && modelSelector.count === 0
        userMessage: root.errorMessage
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }
}
