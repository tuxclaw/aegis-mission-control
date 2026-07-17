pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: root

    property date monthCursor: new Date()
    property date editorDate: new Date()
    property string selectedEventId: ""
    property string selectedColor: "accent"
    property string errorMessage: ""
    property bool retryable: false
    property bool saving: false
    property bool editorOpen: false

    readonly property var paletteTokens: ["accent", "ok", "warn", "alert", "accentDim"]
    readonly property var weekdayNames: [qsTr("MON"), qsTr("TUE"), qsTr("WED"), qsTr("THU"), qsTr("FRI"), qsTr("SAT"), qsTr("SUN")]

    ListView {
        id: eventCounter
        visible: false
        model: calendar.events
    }

    function refreshView() {
        errorMessage = "";
        calendar.refresh();
    }

    function addItem() {
        openNewEditor(new Date());
    }

    function paletteColor(token) {
        return token === "ok" ? Theme.ok : token === "warn" ? Theme.warn : token === "alert" ? Theme.alert : token === "accentDim" ? Theme.accentDim : Theme.accent;
    }

    function cellDate(cellIndex) {
        const first = new Date(monthCursor.getFullYear(), monthCursor.getMonth(), 1);
        const mondayOffset = (first.getDay() + 6) % 7;
        return new Date(first.getFullYear(), first.getMonth(), cellIndex - mondayOffset + 1);
    }

    function sameDay(left, right) {
        const a = new Date(left);
        const b = new Date(right);
        return a.getFullYear() === b.getFullYear() && a.getMonth() === b.getMonth() && a.getDate() === b.getDate();
    }

    function changeMonth(delta) {
        monthCursor = new Date(monthCursor.getFullYear(), monthCursor.getMonth() + delta, 1);
    }

    function openNewEditor(day) {
        selectedEventId = "";
        editorDate = day;
        titleField.text = "";
        startDateField.text = Qt.formatDate(day, "yyyy-MM-dd");
        endDateField.text = Qt.formatDate(day, "yyyy-MM-dd");
        startTimeField.text = "09:00";
        endTimeField.text = "10:00";
        allDayBox.checked = false;
        locationField.text = "";
        notesField.text = "";
        selectedColor = "accent";
        inlineError.text = "";
        editorOpen = true;
    }

    function openExistingEditor(eventId, title, start, end, allDay, location, colorToken, description) {
        selectedEventId = eventId;
        titleField.text = title;
        startDateField.text = Qt.formatDate(start, "yyyy-MM-dd");
        endDateField.text = Qt.formatDate(end, "yyyy-MM-dd");
        startTimeField.text = Qt.formatTime(start, "HH:mm");
        endTimeField.text = Qt.formatTime(end, "HH:mm");
        allDayBox.checked = allDay;
        locationField.text = location;
        notesField.text = description;
        selectedColor = paletteTokens.indexOf(colorToken) >= 0 ? colorToken : "accent";
        inlineError.text = "";
        editorOpen = true;
    }

    function saveEvent() {
        const trimmedTitle = titleField.text.trim();
        const startValue = new Date(startDateField.text + "T" + (allDayBox.checked ? "00:00" : startTimeField.text));
        const endValue = new Date(endDateField.text + "T" + (allDayBox.checked ? "23:59" : endTimeField.text));
        if (trimmedTitle.length < 1 || trimmedTitle.length > 200) {
            inlineError.text = qsTr("Title must be between 1 and 200 characters.");
            return;
        }
        if (isNaN(startValue.getTime()) || isNaN(endValue.getTime()) || endValue < startValue) {
            inlineError.text = qsTr("Enter valid dates with an end at or after the start.");
            return;
        }
        if (locationField.text.length > 200 || notesField.text.length > 4000) {
            inlineError.text = qsTr("Location or notes exceed the allowed length.");
            return;
        }
        inlineError.text = "";
        saving = true;
        if (selectedEventId.length > 0)
            calendar.updateEvent(selectedEventId, trimmedTitle, startValue, endValue, allDayBox.checked, locationField.text, selectedColor, notesField.text);
        else
            calendar.createEvent(trimmedTitle, startValue, endValue, allDayBox.checked, locationField.text, selectedColor, notesField.text);
    }

    Connections {
        target: calendar
        function onErrorRaised(message, canRetry) {
            root.saving = false;
            if (root.editorOpen)
                inlineError.text = message;
            else {
                root.errorMessage = message;
                root.retryable = canRetry;
            }
        }
        function onToast(message, level) {
            root.saving = false;
            ToastHost.show(message, level);
            if (level !== "error")
                root.editorOpen = false;
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Theme.viewGutter

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.space.md

            GhostButton {
                implicitWidth: Theme.compactButtonSize
                text: "‹"
                Accessible.name: qsTr("Previous month")
                onClicked: root.changeMonth(-1)
            }
            Text {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: Qt.formatDate(root.monthCursor, "MMMM yyyy")
                color: Theme.textPrimary
                font.family: Typography.heading.family
                font.pixelSize: Typography.heading.pixelSize
                font.weight: Typography.heading.weight
            }
            GhostButton {
                implicitWidth: Theme.compactButtonSize
                text: "›"
                Accessible.name: qsTr("Next month")
                onClicked: root.changeMonth(1)
            }
            PrimaryButton {
                text: qsTr("+ Event")
                Accessible.name: qsTr("Create calendar event")
                onClicked: root.addItem()
            }
        }

        GlassCard {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                width: parent.width
                height: parent.height
                spacing: Theme.space.sm

                RowLayout {
                    Layout.fillWidth: true
                    Repeater {
                        model: root.weekdayNames
                        Text {
                            required property string modelData
                            Layout.fillWidth: true
                            text: modelData
                            color: Theme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            font.family: Typography.caption.family
                            font.pixelSize: Typography.caption.pixelSize
                            font.weight: Typography.caption.weight
                            font.letterSpacing: Typography.caption.letterSpacing
                        }
                    }
                }

                GridView {
                    id: monthGrid
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: 42
                    cellWidth: width / 7
                    cellHeight: height / 6
                    interactive: false
                    clip: true

                    delegate: Rectangle {
                        id: dayCell
                        required property int index
                        readonly property date dateValue: root.cellDate(index)
                        readonly property bool inMonth: dateValue.getMonth() === root.monthCursor.getMonth()
                        readonly property bool today: root.sameDay(dateValue, new Date())
                        width: monthGrid.cellWidth - Theme.borderWidth
                        height: monthGrid.cellHeight - Theme.borderWidth
                        color: dayTap.hovered ? Theme.accentSoft : Theme.transparent
                        border.width: Theme.borderWidth
                        border.color: Theme.divider
                        Accessible.name: qsTr("Create event on %1").arg(Qt.formatDate(dayCell.dateValue, "MMMM d"))
                        Accessible.role: Accessible.Button

                        Rectangle {
                            anchors {
                                top: parent.top
                                right: parent.right
                                margins: Theme.space.sm
                            }
                            width: Theme.compactButtonSize
                            height: width
                            radius: width / 2
                            color: Theme.transparent
                            border.width: dayCell.today ? Theme.focusBorderWidth : 0
                            border.color: Theme.accent
                            Text {
                                anchors.centerIn: parent
                                text: dayCell.dateValue.getDate()
                                color: dayCell.inMonth ? Theme.textPrimary : Theme.textMuted
                                font.family: Typography.dataSmall.family
                                font.pixelSize: Typography.dataSmall.pixelSize
                                font.weight: Typography.dataSmall.weight
                            }
                        }

                        ListView {
                            id: dayEvents
                            anchors {
                                left: parent.left
                                right: parent.right
                                top: parent.top
                                bottom: parent.bottom
                                margins: Theme.space.sm
                                topMargin: Theme.compactButtonSize + Theme.space.sm
                            }
                            model: calendar.events
                            spacing: Theme.space.xs
                            clip: true
                            interactive: false

                            delegate: Rectangle {
                                id: eventChip
                                required property var model
                                property string eventId: model.id
                                required property string title
                                required property var start
                                required property var end
                                required property bool allDay
                                required property string location
                                property string colorToken: model.color
                                required property string description
                                readonly property bool onThisDay: root.sameDay(start, dayCell.dateValue)
                                width: dayEvents.width
                                height: onThisDay ? Theme.chipHeight : 0
                                visible: onThisDay
                                radius: Theme.radiusControl
                                color: root.paletteColor(eventChip.colorToken)
                                opacity: onThisDay ? 1 : 0
                                Accessible.name: qsTr("Edit event %1").arg(eventChip.title)
                                Accessible.role: Accessible.Button

                                Text {
                                    anchors {
                                        fill: parent
                                        leftMargin: Theme.space.sm
                                        rightMargin: Theme.space.sm
                                    }
                                    verticalAlignment: Text.AlignVCenter
                                    text: eventChip.title
                                    color: Theme.bg
                                    elide: Text.ElideRight
                                    font.family: Typography.caption.family
                                    font.pixelSize: Typography.caption.pixelSize
                                    font.weight: Typography.caption.weight
                                }
                                TapHandler {
                                    onTapped: root.openExistingEditor(eventChip.eventId, eventChip.title, eventChip.start, eventChip.end, eventChip.allDay, eventChip.location, eventChip.colorToken, eventChip.description)
                                }
                            }
                        }

                        HoverHandler {
                            id: dayTap
                        }
                        TapHandler {
                            onTapped: root.openNewEditor(dayCell.dateValue)
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.editorOpen
        color: Theme.modalBackdrop
        z: 20
        Accessible.name: qsTr("Close event editor")
        Accessible.role: Accessible.Button
        TapHandler {
            onTapped: root.editorOpen = false
        }
    }

    GlassCard {
        id: editorDrawer
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        width: Math.min(Theme.editorDrawerWidth, root.width - Theme.space.xxl)
        z: 21
        visible: x < root.width
        x: root.editorOpen ? root.width - width : root.width + Theme.enterOffset
        title: root.selectedEventId.length > 0 ? qsTr("EDIT EVENT") : qsTr("NEW EVENT")

        Behavior on x {
            NumberAnimation {
                duration: Motion.drawer
                easing.type: Motion.drawerEasing
            }
        }

        ScrollView {
            width: parent.width
            height: parent.height
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: Theme.space.md

                TextField {
                    id: titleField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Title")
                    maximumLength: 200
                    Accessible.name: qsTr("Event title")
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: startDateField
                        Layout.fillWidth: true
                        placeholderText: qsTr("Start date")
                        Accessible.name: qsTr("Start date")
                    }
                    TextField {
                        id: startTimeField
                        Layout.preferredWidth: Theme.buttonMinWidth
                        enabled: !allDayBox.checked
                        placeholderText: qsTr("Time")
                        Accessible.name: qsTr("Start time")
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    TextField {
                        id: endDateField
                        Layout.fillWidth: true
                        placeholderText: qsTr("End date")
                        Accessible.name: qsTr("End date")
                    }
                    TextField {
                        id: endTimeField
                        Layout.preferredWidth: Theme.buttonMinWidth
                        enabled: !allDayBox.checked
                        placeholderText: qsTr("Time")
                        Accessible.name: qsTr("End time")
                    }
                }
                CheckBox {
                    id: allDayBox
                    text: qsTr("All day")
                    Accessible.name: qsTr("All-day event")
                }
                TextField {
                    id: locationField
                    Layout.fillWidth: true
                    placeholderText: qsTr("Location")
                    maximumLength: 200
                    Accessible.name: qsTr("Event location")
                }
                Text {
                    text: qsTr("COLOR")
                    color: Theme.textMuted
                    font.family: Typography.caption.family
                    font.pixelSize: Typography.caption.pixelSize
                    font.weight: Typography.caption.weight
                }
                RowLayout {
                    Repeater {
                        model: root.paletteTokens
                        Rectangle {
                            required property string modelData
                            width: Theme.compactButtonSize
                            height: width
                            radius: width / 2
                            color: root.paletteColor(modelData)
                            border.width: root.selectedColor === modelData ? Theme.focusBorderWidth : Theme.borderWidth
                            border.color: root.selectedColor === modelData ? Theme.textPrimary : Theme.panelBorder
                            Accessible.name: qsTr("Use %1 event color").arg(modelData)
                            Accessible.role: Accessible.Button
                            TapHandler {
                                onTapped: root.selectedColor = modelData
                            }
                        }
                    }
                }
                TextArea {
                    id: notesField
                    Layout.fillWidth: true
                    Layout.minimumHeight: Theme.textAreaMinimumHeight
                    placeholderText: qsTr("Notes")
                    wrapMode: TextArea.Wrap
                    Accessible.name: qsTr("Event notes")
                }
                Text {
                    id: inlineError
                    Layout.fillWidth: true
                    visible: text.length > 0
                    color: Theme.alert
                    wrapMode: Text.Wrap
                    font.family: Typography.body.family
                    font.pixelSize: Typography.body.pixelSize
                    font.weight: Typography.body.weight
                }
                RowLayout {
                    Layout.fillWidth: true
                    GhostButton {
                        text: qsTr("Cancel")
                        Accessible.name: qsTr("Cancel event editing")
                        onClicked: root.editorOpen = false
                    }
                    GhostButton {
                        visible: root.selectedEventId.length > 0
                        destructive: true
                        text: qsTr("Delete")
                        Accessible.name: qsTr("Delete event")
                        onClicked: deleteDialog.open()
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    PrimaryButton {
                        text: qsTr("Save Event")
                        busy: root.saving
                        Accessible.name: qsTr("Save event")
                        onClicked: root.saveEvent()
                    }
                }
            }
        }
    }

    LoadingState {
        anchors.fill: parent
        visible: calendar.loading
    }
    EmptyState {
        anchors.centerIn: parent
        visible: !calendar.loading && eventCounter.count === 0 && root.errorMessage.length === 0 && !root.editorOpen
        title: qsTr("No events this month")
        actionText: qsTr("Create event")
        onActionTriggered: root.addItem()
    }
    ErrorState {
        anchors.centerIn: parent
        width: Math.min(parent.width - Theme.space.xxl, Theme.dialogWidth)
        visible: root.errorMessage.length > 0
        userMessage: qsTr("Calendar data could not be read — not overwritten")
        retryable: root.retryable
        onRetryRequested: root.refreshView()
    }

    ConfirmDialog {
        id: deleteDialog
        action: qsTr("Delete calendar event?")
        detail: qsTr("This event will be removed after controller validation.")
        destructive: true
        onAccepted: {
            root.saving = true;
            calendar.deleteEvent(root.selectedEventId);
        }
    }
}
