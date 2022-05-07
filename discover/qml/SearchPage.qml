/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.14 as Kirigami
import QtGraphicalEffects 1.12

ApplicationsListPage {
    id: searchPage
    searchPage: true

    signal shown()
    Timer {
        interval: 0
        running: true
        onTriggered: {
            searchPage.shown()
        }
    }
    
    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.ToolBar
    
    titleDelegate: Controls.Control {
        Layout.fillWidth: true
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0
        
        z: 100
        
        contentItem: SearchField {
            id: searchField
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

    topPadding: 0
}
