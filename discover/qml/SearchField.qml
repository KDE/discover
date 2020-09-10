/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2019 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

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
        currentSearchText = searchField.text
    }

    function clearText() {
        searchField.text = ""
        searchField.accepted()
    }

    Connections {
        ignoreUnknownSignals: true
        target: page
        function onClearSearch() {
            clearText()
        }
    }

    Connections {
        target: applicationWindow()
        function onCurrentTopLevelChanged() {
            if (applicationWindow().currentTopLevel.length > 0)
                clearText()
        }
    }
}
