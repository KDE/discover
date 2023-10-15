/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami

Kirigami.BasicListItem {
    id: controlRoot

    separatorVisible: false
    visible: enabled

    Keys.onEnterPressed: trigger()
    Keys.onReturnPressed: trigger()

    function trigger() {
        if (enabled) {
            if (typeof drawer !== "undefined") {
                drawer.resetMenu()
            }
            action.trigger()
        }
    }

    Kirigami.MnemonicData.enabled: enabled && visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    // Note: normally text is sync'ed with action along with other properties,
    // but unlike other properties if text is set explicitly, it won't be
    // written back to action, so this is technically not a binding loop.
    // Implementation of controls normally binds richTextLabel directly to
    // Text, but in overrides that don't implement style directly, this is
    // our only option left.
    Kirigami.MnemonicData.label: action.text
    text: Kirigami.MnemonicData.richTextLabel

    QQC2.ToolTip.text: shortcut.nativeText
    QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? down : hovered) && shortcut.nativeText.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    Shortcut {
        id: shortcut
        sequence: controlRoot.Kirigami.MnemonicData.sequence
        onActivated: controlRoot.trigger()
    }

    // Using the generic onPressed so individual instances can override
    // behaviour using Keys.on{Up,Down}Pressed
    Keys.onPressed: event => {
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
