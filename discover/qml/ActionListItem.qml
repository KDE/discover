/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Controls 2.5 as QQC2

import org.kde.kirigami 2.14 as Kirigami

Kirigami.BasicListItem
{
    id: item
    property QtObject action: null
    checked: action.checked
    icon: action.iconName
    separatorVisible: false
    visible: action.enabled
    onClicked: {
        drawer.resetMenu()
        action.trigger()
    }

    Kirigami.MnemonicData.enabled: item.enabled && item.visible
    Kirigami.MnemonicData.controlType: Kirigami.MnemonicData.MenuItem
    Kirigami.MnemonicData.label: action.text
    label: Kirigami.MnemonicData.richTextLabel

    readonly property var tooltip: QQC2.ToolTip {
        text: action.shortcut ? action.shortcut : p0.nativeText
    }

    readonly property var p0: Shortcut {
        sequence: item.Kirigami.MnemonicData.sequence
        onActivated: item.clicked()
    }
}
