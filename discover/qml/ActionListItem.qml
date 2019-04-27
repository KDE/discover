/***************************************************************************
 *   Copyright © 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.5
import QtQuick.Controls 2.0
import org.kde.kirigami 2.3 as Kirigami

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

    ToolTip.visible: hovered
    ToolTip.text: action.shortcut ? action.shortcut : p0.nativeText

    readonly property var p0: Shortcut {
        sequence: item.Kirigami.MnemonicData.sequence
        onActivated: item.clicked()
    }
}
