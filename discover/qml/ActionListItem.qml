/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

QQC2.ItemDelegate {
    id: item

    Layout.fillWidth: true

    highlighted: checked
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

    property string subtitle
    property string stateIconName

    contentItem: RowLayout {
        spacing: Kirigami.Units.largeSpacing
        KD.IconTitleSubtitle {
            Layout.fillWidth: true
            icon: icon.fromControlsIcon(item.icon)
            title: item.text
            subtitle: item.subtitle
            selected: item.highlighted
            font: item.font
        }
        Kirigami.Icon {
            Layout.fillHeight: true
            visible: item.stateIconName.length > 0
            source: item.stateIconName
            implicitWidth: Kirigami.Units.iconSizes.sizeForLabels
            implicitHeight: Kirigami.Units.iconSizes.sizeForLabels
        }
    }

    Kirigami.MnemonicData.enabled: item.enabled && item.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action.text

    // Note changing text here does not affect the action.text
    text: Kirigami.MnemonicData.richTextLabel

    QQC2.ToolTip.text: shortcut.nativeText
    QQC2.ToolTip.visible: (Kirigami.Settings.tabletMode ? down : hovered) && QQC2.ToolTip.text.length > 0
    QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

    Shortcut {
        id: shortcut
        sequence: item.Kirigami.MnemonicData.sequence
        onActivated: item.trigger()
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
