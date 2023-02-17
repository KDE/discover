/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.14 as Kirigami

Kirigami.BasicListItem {
    id: item

    property QtObject action: null

    checked: action.checked
    icon: action.iconName
    separatorVisible: false
    visible: action.enabled

    onClicked: trigger()
    Keys.onEnterPressed: trigger()
    Keys.onReturnPressed: trigger()

    function trigger() {
        drawer.resetMenu()
        action.trigger()
    }

    Kirigami.MnemonicData.enabled: item.enabled && item.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action.text
    label: Kirigami.MnemonicData.richTextLabel

    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
    QQC2.ToolTip.visible: hovered && p0.nativeText.length > 0
    QQC2.ToolTip.text: p0.nativeText

    readonly property var p0: Shortcut {
        sequence: item.Kirigami.MnemonicData.sequence
        onActivated: item.clicked()
    }

    // Using the generic onPressed so individual instances can override
    // behaviour using Keys.on{Up,Down}Pressed
    Keys.onPressed: {
        if (event.accepted) {
            return
        }

        // Using forceActiveFocus here since the item may be in a focus scope
        // and just setting focus won't focus the scope.
        if (event.key === Qt.Key_Up) {
            nextItemInFocusChain(false).forceActiveFocus()
            event.accepted = true
        } else if (event.key === Qt.Key_Down) {
            nextItemInFocusChain(true).forceActiveFocus()
            event.accepted = true
        }
    }
}
