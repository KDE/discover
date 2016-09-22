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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.GlobalDrawer {
    id: drawer
    bannerImageSource: "qrc:/icons/banner.svg"
    topPadding: -50
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    resetMenuOnTriggered: false

    readonly property var currentRootCategory: window.leftPage ? rootCategory(window.leftPage.category) : null

    onBannerClicked: {
        Navigation.openHome();
    }

    onCurrentSubMenuChanged: {
        if (currentSubMenu)
            currentSubMenu.trigger()
        else if (searchField.text !== "")
            window.leftPage.category = null
        else
            Navigation.openHome()

    }

    topContent: TextField {
        id: searchField
        Layout.fillWidth: true
        Layout.leftMargin: Kirigami.Units.smallSpacing
        Layout.rightMargin: Kirigami.Units.smallSpacing

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
                if (!curr.hasOwnProperty("search")) {
                    Navigation.clearStack()
                    Navigation.openApplicationList( { search: parent.text })
                } else
                    curr.search = parent.text;
            }
        }
    }

    ColumnLayout {
        spacing: 0
        Layout.fillWidth: true

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        ProgressView {
            separatorVisible: false
        }

        Kirigami.BasicListItem {
            checked: installedAction.checked
            icon: installedAction.iconName
            label: installedAction.text
            separatorVisible: false
            onClicked: {
                installedAction.trigger()
                drawer.resetMenu()
            }
        }
        Kirigami.BasicListItem {
            checked: settingsAction.checked
            icon: settingsAction.iconName
            label: settingsAction.text
            separatorVisible: false
            onClicked: {
                settingsAction.trigger()
                drawer.resetMenu()
            }
        }
        Kirigami.BasicListItem {
            enabled: updateAction.enabled
            checked: updateAction.checked
            icon: updateAction.iconName
            label: updateAction.text
            separatorVisible: false
            onClicked: {
                updateAction.trigger()
                drawer.resetMenu()
            }

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

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            property QtObject category
            readonly property bool itsMe: window.leftPage && window.leftPage.hasOwnProperty("category") && (window.leftPage.category == category)
            text: category.name
            checkable: itsMe
            checked: itsMe
            visible: (!window.leftPage
                   || !window.leftPage.subcategories
                   || window.leftPage.subcategories === undefined
                   || searchField.text.length === 0
                   || category.contains(window.leftPage.subcategories)
                     )
            onTriggered: {
                Navigation.openCategory(category, searchField.text)
            }
        }
    }

    function createCategoryActions(parent, categories) {
        var actions = []
        for(var i in categories) {
            var cat = categories[i];
            var catAction = categoryActionComponent.createObject(parent, {category: cat});
            catAction.children = createCategoryActions(catAction, cat.subcategories);
            actions.push(catAction)
        }
        return actions;
    }

    CategoryModel {
        id: rootCategories
        Component.onCompleted: {
            resetCategories();
            drawer.actions = createCategoryActions(rootCategories, rootCategories.categories)
        }
    }

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
