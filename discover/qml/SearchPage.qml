/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5

ApplicationsListPage {
    id: searchPage

    signal shown()
    Timer {
        interval: 0
        running: true
        onTriggered: {
            searchPage.shown()
        }
    }

    listHeaderPositioning: ListView.OverlayHeader
    listHeader: SearchField {
        id: searchField
        width: parent.width
        focus: true
        z: 100
        Component.onCompleted: forceActiveFocus()

        Connections {
            ignoreUnknownSignals: true
            target: searchPage
            function onShown() {
                searchField.forceActiveFocus()
            }
        }

        onCurrentSearchTextChanged: {
            searchPage.search = currentSearchText
        }
    }
}
