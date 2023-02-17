/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2019 Carl Schwan <carl@carlschwan.eu>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Controls 2.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.SearchField {
    id: searchField

    // Search operations are network-intensive, so we can't have search-as-you-type.
    // This means we should turn off auto-accept entirely, rather than having it on
    // with a delay. The result just isn't good. See Bug 445142.
    autoAccept: false

    property QtObject page
    property string currentSearchText

    placeholderText: (!enabled || !page || page.hasOwnProperty("isHome") || window.leftPage.name.length === 0) ? i18n("Search…") : i18n("Search in '%1'…", window.leftPage.name)

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
            if (applicationWindow().currentTopLevel.length > 0) {
                clearText()
            }
        }
    }
}
