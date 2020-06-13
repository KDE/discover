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
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.12 as Kirigami

Kirigami.GlobalDrawer {
    id: drawer

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // FIXME: Dirty workaround for 385992
    width: Kirigami.Units.gridUnit * 14

    property bool wideScreen: false
    bannerImageSource: "qrc:/banners/banner.svg"

    resetMenuOnTriggered: false
    Kirigami.PageRouter.router: window.router

    onBannerClicked: {
        if (window.globalDrawer.currentSubMenu)
            window.globalDrawer.resetMenu()
        Kirigami.PageRouter.navigateToRoute("browsing")
        drawerOpen = false
    }

    property string currentSearchText

    onCurrentSubMenuChanged: {
        if (currentSubMenu)
            currentSubMenu.trigger()
        else if (currentSearchText.length > 0)
            window.leftPage.category = null
        else {
            if (window.globalDrawer.currentSubMenu)
                window.globalDrawer.resetMenu()
            Kirigami.PageRouter.navigateToRoute("browsing")
        }
    }

    function suggestSearchText(text) {
        searchField.text = text
        searchField.forceActiveFocus()
    }

    // Give the search field keyboard focus by default
    Component.onCompleted: {
        searchField.forceActiveFocus();
    }

    header: Kirigami.AbstractApplicationHeader {
        id: toolbar
        visible: drawer.wideScreen

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Kirigami.Units.smallSpacing
            anchors.rightMargin: Kirigami.Units.smallSpacing

            SearchField {
                id: searchField

                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                Layout.fillHeight: true
                Layout.fillWidth: true

                visible: window.leftPage && window.leftPage.hasOwnProperty("search")
                Kirigami.PageRouter.router: window.router

                page: window.leftPage

                onCurrentSearchTextChanged: {
                    var curr = window.leftPage;

                    if (pageStack.depth>1)
                        pageStack.pop()

                    if (currentSearchText === "" && window.currentTopLevel === "" && !window.leftPage.category) {
                        if (window.globalDrawer.currentSubMenu)
                            window.globalDrawer.resetMenu()
                        Kirigami.PageRouter.navigateToRoute("browsing")
                    } else if (!curr.hasOwnProperty("search")) {
                        if (currentSearchText) {
                            Kirigami.PageRouter.navigateToRoute({"route": "application-list", "data": {"search": currentSearchText}})
                        }
                    } else {
                        curr.search = currentSearchText;
                        curr.forceActiveFocus()
                    }
                }
            }

            ToolButton {

                icon.name: "go-home"
                Kirigami.PageRouter.router: window.router
                onClicked: {
                    if (window.globalDrawer.currentSubMenu)
                        window.globalDrawer.resetMenu()
                    Kirigami.PageRouter.navigateToRoute("browsing")
                }

                ToolTip {
                    text: i18n("Return to the Featured page")
                }
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
            action: TopLevelPageData {
                id: searchAction
                enabled: !window.wideScreen
                iconName: "search"
                text: i18n("Search")
                route: "search"
                objectName: "discover"
                shortcut: "Ctrl+F"
            }
        }
        ActionListItem {
            action: TopLevelPageData {
                id: installedAction
                iconName: "view-list-details"
                text: i18n("Installed")
                route: "installed"
                objectName: "installed"
            }
        }
        ActionListItem {
            action: TopLevelPageData {
                id: sourcesAction
                iconName: "configure"
                text: i18n("Settings")
                route: "sources"
                objectName: "sources"
            }
        }

        ActionListItem {
            action: TopLevelPageData {
                id: aboutAction
                iconName: "help-feedback"
                text: i18n("About")
                route: "about"
                objectName: "about"
            }
        }

        ActionListItem {
            objectName: "updateButton"
            action: TopLevelPageData {
                id: updateAction
                iconName: ResourcesModel.updatesCount>0 ? ResourcesModel.hasSecurityUpdates ? "update-high" : "update-low" : "update-none"
                text: ResourcesModel.updatesCount<=0 ? (ResourcesModel.isFetching ? i18n("Fetching updates...") : i18n("Up to date") ) : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
                route: "update"
                objectName: "update"
            }
            backgroundColor: ResourcesModel.updatesCount>0 ? "orange" : Kirigami.Theme.backgroundColor
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

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            property QtObject category
            readonly property bool itsMe: window.leftPage && window.leftPage.hasOwnProperty("category") && (window.leftPage.category === category)
            text: category ? category.name : ""
            iconName: category ? category.icon : ""
            checked: itsMe
            visible: (!window.leftPage
                   || !window.leftPage.subcategories
                   || window.leftPage.subcategories === undefined
                   || currentSearchText.length === 0
                   || (category && category.contains(window.leftPage.subcategories))
                     )
            Kirigami.PageRouter.router: window.router
            onTriggered: {
                Kirigami.PageRouter.navigateToRoute({"route": "application-list", "data": {"category": category, "search": currentSearchText}})
            }
        }
    }

    function createCategoryActions(categories) {
        var actions = []
        for(var i in categories) {
            var cat = categories[i];
            var catAction = categoryActionComponent.createObject(drawer, {category: cat, children: createCategoryActions(cat.subcategories)});
            actions.push(catAction)
        }
        return actions;
    }

    actions: createCategoryActions(CategoryModel.rootCategories)

    modal: !drawer.wideScreen
    handleVisible: !drawer.wideScreen
}
