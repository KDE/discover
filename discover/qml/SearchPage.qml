/*
 *   SPDX-FileCopyrightText: 2017 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

ApplicationsListPage {
    id: root

    signal shown()

    searchPage: true

    Timer {
        interval: 0
        running: true
        onTriggered: {
            root.shown()
        }
    }

    topPadding: 0

    globalToolBarStyle: Kirigami.ApplicationHeaderStyle.ToolBar

    titleDelegate: SearchField {
        id: searchField

        Layout.fillWidth: true

        focus: !window.wideScreen
        visible: !window.wideScreen

        Component.onCompleted: forceActiveFocus()

        Connections {
            target: root
            function onShown() {
                searchField.forceActiveFocus()
            }
        }

        onCurrentSearchTextChanged: {
            root.search = currentSearchText
        }
    }
}
