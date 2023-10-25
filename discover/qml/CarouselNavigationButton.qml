/*
 *   SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Templates as T
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as KirigamiComponents

KirigamiComponents.FloatingButton {
    id: controlRoot

    /*
     * This property controls whether opacity and translation should be animated.
     * Note that any cuurently running animations can not be stopped.
     */
    required property bool animated

    /*
     * Two properties reflecting whether the current item is the first one in
     * carousel, and/or whether it is the last one. Note that they are not
     * mutually exclusive, if the carousel contains only a single item.
     */
    required property bool atBeginning
    required property bool atEnd

    /*
     * Extra offset toward the button's primary direction to add on top of the
     * default padding.
     */
    property real edgeMargin: Kirigami.Units.gridUnit

    property int role: Qt.AlignLeading

    readonly property int effectiveRole: mirrored ? (role === Qt.AlignLeading ? Qt.AlignTrailing : Qt.AlignLeading) : role

    anchors {
        left: role === Qt.AlignLeading ? parent?.left : undefined
        right: role === Qt.AlignTrailing ? parent?.right : undefined
    }

    height: parent?.height ?? undefined

    leftMargin: effectiveRole === Qt.AlignLeading ? edgeMargin : 0
    rightMargin: effectiveRole === Qt.AlignTrailing ? edgeMargin : 0

    radius: Infinity

    icon.name: effectiveRole === Qt.AlignLeading
        ? "arrow-left-symbolic" : "arrow-right-symbolic"

    text: effectiveRole === Qt.AlignLeading
        ? i18nc("@action:button", "Previous Screenshot") : i18nc("@action:button", "Next Screenshot")

    focusPolicy: Qt.NoFocus

    enabled: role === Qt.AlignLeading
        ? !atBeginning : !atEnd

    opacity: enabled ? 1 : 0

    Behavior on opacity {
        enabled: controlRoot.visible && controlRoot.animated
        NumberAnimation {
            duration: Kirigami.Units.longDuration
            easing.type: Easing.OutCubic
        }
    }

    transform: Translate {
        x: controlRoot.enabled ? 0
            : Kirigami.Units.gridUnit * 2 * (controlRoot.effectiveRole === Qt.AlignLeading ? -1 : 1)

        Behavior on x {
            enabled: controlRoot.visible && controlRoot.animated
            NumberAnimation {
                duration: Kirigami.Units.longDuration
                easing.type: Easing.OutCubic
            }
        }
    }
}
