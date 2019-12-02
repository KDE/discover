/***************************************************************************
 *   Copyright © 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *   Copyright © 2019 Carl Schwan <carl@carlschwan.eu>                     *
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
import org.kde.kirigami 2.7 as Kirigami

Kirigami.ActionTextField
{
    id: searchField
    focusSequence: "Ctrl+F"
    rightActions: [
        Kirigami.Action {
            iconName: "edit-clear"
            visible: searchField.text.length !== 0
            onTriggered: searchField.clearText()
        }
    ]

    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || page.title.length === 0) ? i18n("Search...") : i18n("Search in '%1'...", window.leftPage.title)

    onAccepted: {
        searchField.text = searchField.text.replace(/\n/g, ' ');
        console.log("searchField.text", searchField.text)
        currentSearchText = searchField.text
    }

    function clearText() {
        searchField.text = ""
        searchField.accepted()
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
