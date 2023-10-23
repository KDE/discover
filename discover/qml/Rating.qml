/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

QQC2.Control {
    id: control

    enum EditPolicy {
        None = 0,
        AllowTapToUnset = 1,
        AllowSetZero = 2
    }

    enum Precision {
        FullStar,
        HalfStar
    }

    property real starSize: Kirigami.Units.gridUnit
    property bool readOnly: true
    property /*EditPolicy*/ int editPolicy: Rating.EditPolicy.None
    property int /*Precision*/ precision: Rating.Precision.FullStar

    // Allows gracefully handling edits in FullStar precision starting from a HalfStar value.
    // For example, starting from value 3 (one and a half star, displayed as 2 full stars),
    // with AllowTapToUnset policy clicking on second star will reset value to 0.
    readonly property int effectiveValue: precision === Rating.Precision.HalfStar ? value : Math.ceil(value / 2) * 2

    readonly property int hoveredValue: mouseArea.hoveredValue
    readonly property bool pressed: mouseArea.pressed

    // Accessible API requires Slider types to have properties named literally like this.
    property int value: 0
    readonly property int minimumValue: 0
    property int maximumValue: 10
    readonly property int stepSize: precision === Rating.Precision.HalfStar ? 1 : 2

    signal edited()

    function valueAt(position: real): int {
        position = Math.max(0, Math.min(1, position));
        let val = 0;
        let step = 0;
        switch (precision) {
        case Rating.Precision.HalfStar:
            val = Math.ceil(position * maximumValue);
            step = 1;
            break;
        case Rating.Precision.FullStar:
            val = Math.ceil(position * maximumValue / 2) * 2;
            step = 2;
            break;
        }
        const min = (editPolicy & Rating.EditPolicy.AllowSetZero) ? 0 : step;
        val = Math.max(min, val);
        return val;
    }

    function __edit(newValue: int) {
        if (newValue === value) {
            return;
        }
        value = newValue;
        edited();
    }

    function decrease() {
        if (!readOnly) {
            const step = precision === Rating.Precision.HalfStar ? 1 : 2;
            const min = (editPolicy & Rating.EditPolicy.AllowSetZero) ? 0 : step;
            const newValue = Math.max(min, effectiveValue - step);
            __edit(newValue);
        }
    }

    function increase() {
        if (!readOnly) {
            const step = precision === Rating.Precision.HalfStar ? 1 : 2;
            const newValue = Math.min(maximumValue, effectiveValue + step);
            __edit(newValue);
        }
    }

    Keys.onLeftPressed: event => {
        if (readOnly) {
            event.accepted = false;
        } else {
            event.accepted = true;
            if (control.mirrored) {
                increase();
            } else {
                decrease();
            }
        }
    }

    Keys.onRightPressed: event => {
        if (readOnly) {
            event.accepted = false;
        } else {
            event.accepted = true;
            if (control.mirrored) {
                decrease();
            } else {
                increase();
            }
        }
    }

    Accessible.role: Accessible.Slider
    Accessible.description: i18n("Rating")
    Accessible.onIncreaseAction: increase()
    Accessible.onDecreaseAction: decrease()

    focusPolicy: readOnly ? Qt.NoFocus : Qt.StrongFocus

    hoverEnabled: !readOnly

    padding: 0
    // Reset paddings after qqc2-desktop-style Control
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    spacing: 0

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight

        Row {
            id: row

            spacing: 0

            LayoutMirroring.enabled: control.mirrored

            Repeater {
                model: Math.ceil(control.maximumValue / 2)

                Kirigami.Icon {
                    required property int index

                    width: control.starSize
                    height: control.starSize

                    animated: false

                    source: {
                        const base = index * 2;
                        const rating = !control.readOnly && control.hovered ? control.hoveredValue : control.effectiveValue;
                        if (rating <= base) {
                            return "rating-unrated";
                        } else if (rating === base + 1 && control.precision === Rating.Precision.HalfStar) {
                            return control.mirrored ? "rating-half-rtl" : "rating-half";
                        } else {
                            // rating >= base + 2
                            return "rating";
                        }
                    }

                    opacity: {
                        const base = index * 2;
                        const rating = !control.readOnly && control.hovered ? control.hoveredValue : control.effectiveValue;
                        if (rating <= base) {
                            return 1;
                        } else if (!control.readOnly && control.hovered && (control.pressed || control.hoveredValue !== control.effectiveValue)) {
                            return 0.7;
                        } else {
                            return 1;
                        }
                    }
                }
            }
        }
    }

    // Spans entire control, accounts for paddings
    MouseArea {
        id: mouseArea

        anchors.fill: parent

        enabled: !control.readOnly

        acceptedButtons: Qt.LeftButton
        hoverEnabled: true
        // Event stealing prevention seem to be required for it to work in Kirigami.OverlaySheet dialog.
        preventStealing: true

        property int hoveredValue: 0

        // Need to differentiate between press+drag vs click
        property bool dragging: false
        property int dragStartValue: -1

        function initDrag(x: real) {
            const value = valueAt(x);
            dragStartValue = value;
            dragging = false;
        }

        function updateDrag(value: int) {
            if (value !== dragStartValue) {
                dragging = true;
            }
        }

        function resetDrag() {
            dragStartValue = -1;
            dragging = false;
        }

        function positionAt(x: real): real {
            const visualPosition = (x - control.leftPadding) / (control.width - control.leftPadding - control.rightPadding);
            const position = control.mirrored ? 1 - visualPosition : visualPosition;
            return position;
        }

        function valueAt(x: real): int {
            const position = positionAt(x);
            const value = control.valueAt(position);
            return value;
        }

        function setValueAt(x: real) {
            let value = valueAt(x);
            if (!dragging && (control.editPolicy & Rating.EditPolicy.AllowTapToUnset) && (value === control.effectiveValue)) {
                value = 0;
            }
            control.__edit(value);
        }

        function handleMove(x: real) {
            const value = valueAt(mouseX);
            hoveredValue = value;
            if (pressed) {
                updateDrag(value);
            }
        }

        onPressed: mouse => {
            initDrag(mouse.x);
        }

        onReleased: mouse => {
            setValueAt(mouse.x);
            resetDrag();
        }

        onEntered: {
            // In some situations entered() signal may not be immediately
            // followed by positionChanged(), leading to desync with
            // control which does react to mouse enter appropriately.
            // Fix this by handling entered() and reading mouseX property.
            handleMove(mouseX);
        }

        onPositionChanged: mouse => {
            handleMove(mouse.x);
        }

        onExited: {
            hoveredValue = 0;
            resetDrag();
        }

        onCanceled: {
            hoveredValue = 0;
            resetDrag();
        }
    }

    background: Rectangle {
        color: "transparent"
        border.color: control.Kirigami.Theme.highlightColor
        border.width: 1
        radius: Kirigami.Units.smallSpacing

        opacity: control.activeFocus && [Qt.TabFocusReason, Qt.BacktabFocusReason].includes(control.focusReason)
            ? 1 : 0

        Behavior on opacity {
            OpacityAnimator {
                duration: Kirigami.Units.shortDuration
                easing.type: Easing.InOutCubic
            }
        }
    }
}
