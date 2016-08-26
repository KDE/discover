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

import QtQuick 2.7
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.GlobalDrawer {
    id: drawer
    title: i18n("Discover")
    titleIcon: "plasmadiscover"
    bannerImageSource: "image://icon/plasmadiscover"
    topPadding: -50
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    readonly property var currentRootCategory: window.leftPage ? rootCategory(window.leftPage.category) : null

    topContent: TextField {
        id: searchField
        Layout.fillWidth: true

        enabled: window.leftPage && (window.leftPage.searchFor != null || window.leftPage.hasOwnProperty("search"))

        Component.onCompleted: {
            searchField.forceActiveFocus()
        }
        Shortcut {
            sequence: "Ctrl+F"
            onActivated: {
                searchField.forceActiveFocus()
            }
        }

        placeholderText: (!enabled || window.leftPage.title.length === 0) ? i18n("Search...") : i18n("Search in '%1'...", window.leftPage.title)
        onTextChanged: searchTimer.running = true

        Connections {
            ignoreUnknownSignals: true
            target: window.leftPage
            onClearSearch: {
                searchField.text = ""
//                 console.log("search cleared")
            }
        }

        Timer {
            id: searchTimer
            running: false
            repeat: false
            interval: 200
            onTriggered: {
                var curr = window.leftPage;
                if (!curr.hasOwnProperty("search"))
                    Navigation.openApplicationList( { search: parent.text })
                else
                    curr.search = parent.text;
            }
        }
    }

    ColumnLayout {
        spacing: 0
        Layout.fillWidth: true

        ProgressView {}

        Kirigami.BasicListItem {
            checked: installedAction.checked
            icon: installedAction.iconName
            label: installedAction.text
            onClicked: installedAction.trigger()
        }
        Kirigami.BasicListItem {
            checked: settingsAction.checked
            icon: settingsAction.iconName
            label: settingsAction.text
            onClicked: settingsAction.trigger()
        }
        Kirigami.BasicListItem {
            enabled: updateAction.enabled
            checked: updateAction.checked
            icon: updateAction.iconName
            label: updateAction.text
            onClicked: updateAction.trigger()

            backgroundColor: enabled ? "orange" : Kirigami.Theme.viewBackgroundColor
        }
    }

    function rootCategory(cat) {
        var ret = null
        while (cat) {
            ret = cat
            cat = cat.parent
        }
        return ret
    }

    property var objects: []
    Instantiator {
        model: CategoryModel {
            Component.onCompleted: resetCategories()
        }
        delegate: Kirigami.Action {
            text: display
            onTriggered: Navigation.openCategory(category, "")
            checkable: true
            checked: drawer.currentRootCategory == category
        }

        onObjectAdded: {
            drawer.objects.push(object)
            drawer.actions = drawer.objects
        }
        onObjectRemoved: {
            drawer.objects.splice(drawer.objects.indexOf(object), 1)
            drawer.objects = drawer.objects;
        }
    }

    actions: objects
    modal: Helpers.isCompact
    handleVisible: Helpers.isCompact

    states: [
        State {
            name: "full"
            when: !Helpers.isCompact
            PropertyChanges { target: drawer; opened: true }
        }
    ]
}
