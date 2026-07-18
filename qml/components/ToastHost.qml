pragma Singleton
pragma ComponentBehavior: Bound

import QtQuick
import "../theme"

Item {
    id: root

    property Item host: null
    parent: host
    width: host ? host.width : 0
    height: host ? host.height : 0
    visible: host !== null
    z: 1000

    function show(message, level) {
        const normalizedLevel = level || "info";
        toastModel.append({
            "toastMessage": message,
            "toastLevel": normalizedLevel,
            "timeoutMs": normalizedLevel === "error" ? Theme.errorToastDuration : Theme.toastDuration
        });
    }

    function info(message) {
        show(message, "info");
    }
    function success(message) {
        show(message, "success");
    }
    function warn(message) {
        show(message, "warn");
    }
    function error(message) {
        show(message, "error");
    }

    ListModel {
        id: toastModel
    }

    ListView {
        id: toastList
        anchors {
            right: parent.right
            bottom: parent.bottom
            rightMargin: Theme.space.xl
            bottomMargin: Theme.statusBarHeight + Theme.space.xl
        }
        width: Math.min(Theme.toastHostWidth, root.width - Theme.space.xxl)
        height: Math.min(contentHeight, root.height - Theme.statusBarHeight - Theme.space.xxl)
        spacing: Theme.toastStackSpacing
        verticalLayoutDirection: ListView.BottomToTop
        interactive: contentHeight > height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        model: toastModel

        delegate: Rectangle {
            id: toastDelegate

            required property string toastMessage
            required property string toastLevel
            required property int timeoutMs
            required property int index

            readonly property color levelColor: toastLevel === "success" ? Theme.ok : toastLevel === "warn" ? Theme.warn : toastLevel === "error" ? Theme.alert : Theme.accent

            width: toastList.width
            implicitHeight: Math.max(Theme.toastMinHeight, toastText.implicitHeight + Theme.space.lg * 2)
            color: Theme.panel
            radius: Theme.radiusControl
            border.width: Theme.borderWidth
            border.color: levelColor
            opacity: 0

            transform: Translate {
                id: toastTranslation
                x: Theme.toastWidth
            }

            Rectangle {
                width: Theme.activeBarWidth
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                }
                color: toastDelegate.levelColor
                radius: Theme.radiusPill
            }

            Text {
                id: toastText
                anchors {
                    left: parent.left
                    right: dismissButton.left
                    verticalCenter: parent.verticalCenter
                    leftMargin: Theme.space.lg
                    rightMargin: Theme.space.md
                }
                text: toastDelegate.toastMessage
                color: Theme.textPrimary
                wrapMode: Text.Wrap
                maximumLineCount: 4
                elide: Text.ElideRight
                font.family: Typography.body.family
                font.pixelSize: Typography.body.pixelSize
                font.weight: Typography.body.weight
                font.letterSpacing: Typography.body.letterSpacing
            }

            GhostButton {
                id: dismissButton
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                    rightMargin: Theme.space.sm
                }
                implicitWidth: Theme.compactButtonSize
                text: "×"
                Accessible.name: qsTr("Dismiss notification")
                onClicked: toastDelegate.dismiss()
            }

            function dismiss() {
                if (!exitAnimation.running)
                    exitAnimation.start();
            }

            Timer {
                interval: toastDelegate.timeoutMs
                running: true
                onTriggered: toastDelegate.dismiss()
            }

            ParallelAnimation {
                id: enterAnimation
                NumberAnimation {
                    target: toastDelegate
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: Motion.toast
                    easing.type: Motion.toastEasing
                }
                NumberAnimation {
                    target: toastTranslation
                    property: "x"
                    from: Theme.toastWidth
                    to: 0
                    duration: Motion.toast
                    easing.type: Motion.toastEasing
                }
            }

            ParallelAnimation {
                id: exitAnimation
                NumberAnimation {
                    target: toastDelegate
                    property: "opacity"
                    to: 0
                    duration: Motion.fast
                    easing.type: Motion.fastEasing
                }
                NumberAnimation {
                    target: toastTranslation
                    property: "x"
                    to: Theme.toastWidth
                    duration: Motion.fast
                    easing.type: Motion.fastEasing
                }
                onStopped: toastModel.remove(toastDelegate.index)
            }

            Component.onCompleted: enterAnimation.start()
        }
    }
}
