/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
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
import QtQuick.Controls 2.1
import org.kde.kirigami 2.1 as Kirigami

TextField
{
    id: searchField
    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || page.title.length === 0) ? i18n("Search...") : i18n("Search in '%1'...", window.leftPage.title)

    Shortcut {
        sequence: "Ctrl+F"
        onActivated: {
            searchField.forceActiveFocus()
            searchField.selectAll()
        }
    }
    onAccepted: {
        currentSearchText = text
    }

    hoverEnabled: true
    ToolTip {
        delay: Kirigami.Units.longDuration
        visible: hovered && searchField.text.length === 0
        text: searchAction.shortcut
    }

    function clearText() {
        searchField.text = ""
        searchField.accepted()
    }

    ToolButton {
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
            margins: Kirigami.Units.smallSpacing
        }
        icon.name: "edit-clear"
        visible: searchField.text != ""
        onClicked: clearText()
    }

    Connections {
        ignoreUnknownSignals: true
        target: page
        onClearSearch: clearText()
    }

    Connections {
        target: applicationWindow()
        onCurrentTopLevelChanged: {
            if (applicationWindow().currentTopLevel.length > 0)
                clearText()
        }
    }
}
