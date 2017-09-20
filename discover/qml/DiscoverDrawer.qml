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
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.GlobalDrawer {
    id: drawer
    property bool wideScreen: false
    bannerImageSource: "qrc:/banners/banner.svg"
    //make the left and bottom margins for search field the same
    topPadding: searchField.visible ? -searchField.height - leftPadding : 0
    bottomPadding: 0

    resetMenuOnTriggered: false

    onBannerClicked: {
        Navigation.openHome();
    }

    function clearSearch() {
        searchField.text = ""
    }

    onCurrentSubMenuChanged: {
        if (currentSubMenu)
            currentSubMenu.trigger()
        else if (searchField.text !== "")
            window.leftPage.category = null
        else
            Navigation.openHome()

    }

    topContent: drawer.wideScreen ? searchField : null
    TextField {
        id: searchField
        Layout.fillWidth: true
        visible: drawer.wideScreen

        enabled: window.leftPage && (window.leftPage.searchFor != null || window.leftPage.hasOwnProperty("search"))
        Keys.forwardTo: [window.pageStack]

        Component.onCompleted: {
            searchField.forceActiveFocus()
        }
        Shortcut {
            sequence: "Ctrl+F"
            onActivated: {
                searchField.forceActiveFocus()
                searchField.selectAll()
            }
        }

        placeholderText: (!enabled || !window.leftPage || window.leftPage.title.length === 0) ? i18n("Search...") : i18n("Search in '%1'...", window.leftPage.title)
        onTextChanged: {
            if(window.stack.depth > 0)
                searchTimer.running = true
        }

        Connections {
            ignoreUnknownSignals: true
            target: window.leftPage
            onClearSearch: {
                drawer.clearSearch()
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
        Layout.leftMargin: -drawer.leftPadding
        Layout.rightMargin: -drawer.rightPadding

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        ProgressView {
            separatorVisible: false
        }

        ActionListItem {
            action: searchAction
        }
        ActionListItem {
            action: installedAction
        }
        ActionListItem {
            action: settingsAction
        }
        ActionListItem {
            objectName: "updateButton"
            action: updateAction

            backgroundColor: ResourcesModel.updatesCount>0 ? "orange" : Kirigami.Theme.viewBackgroundColor
        }

        states: [
            State {
                name: "full"
                when: drawer.wideScreen
                PropertyChanges { target: drawer; drawerOpen: true }
            },
            State {
                name: "compact"
                when: !drawer.wideScreen
                PropertyChanges { target: drawer; drawerOpen: false }
            }
        ]
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
            checked: itsMe
            visible: (!window.leftPage
                   || !window.leftPage.subcategories
                   || window.leftPage.subcategories === undefined
                   || searchField.text.length === 0
                   || (category && category.contains(window.leftPage.subcategories))
                     )
            onTriggered: {
                if (!window.leftPage.canNavigate)
                    Navigation.openCategory(category, searchField.text)
                else {
                    window.leftPage.category = category
                    pageStack.currentIndex = 0
                }
            }
        }
    }

    function createCategoryActions(categories) {
        var actions = []
        for(var i in categories) {
            var cat = categories[i];
            var catAction = categoryActionComponent.createObject(drawer, {category: cat});
            catAction.children = createCategoryActions(cat.subcategories);
            actions.push(catAction)
        }
        return actions;
    }

    actions: createCategoryActions(CategoryModel.rootCategories)

    modal: !drawer.wideScreen
    handleVisible: !drawer.wideScreen
}
