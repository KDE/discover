/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.5
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.19 as Kirigami
import "navigation.js" as Navigation

Kirigami.GlobalDrawer {
    id: drawer

    property bool wideScreen: false
    property string currentSearchText

    function suggestSearchText(text) {
        if (searchField.visible) {
            searchField.text = text
            forceSearchFieldFocus()
        }
    }

    function forceSearchFieldFocus() {
        if (searchField.visible && wideScreen) {
            searchField.forceActiveFocus();
        }
    }

    function createCategoryActions(categories) {
        var ret = []
        for (var x in categories) {
            var y = categoryActionComponent.createObject(drawer, {
                category: categories[x]
            })
            y.children = createCategoryActions(categories[x].subcategories)
            ret.push(y)
        }
        return ret;
    }
    actions: createCategoryActions(CategoryModel.rootCategories)

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // FIXME: Dirty workaround for 385992
    width: Kirigami.Units.gridUnit * 14

    resetMenuOnTriggered: false
    modal: !drawer.wideScreen

    onCurrentSubMenuChanged: {
        if (currentSubMenu) {
            currentSubMenu.trigger()
        } else if (currentSearchText.length > 0) {
            window.leftPage.category = null
        } else {
            Navigation.openHome()
        }
    }

    header: Kirigami.AbstractApplicationHeader {
        visible: drawer.wideScreen

        contentItem: SearchField {
            id: searchField

            anchors {
                left: parent.left
                leftMargin: Kirigami.Units.smallSpacing
                right: parent.right
                rightMargin: Kirigami.Units.smallSpacing
            }

            // Give the search field keyboard focus by default, unless it would
            // make the virtual keyboard appear, because we don't want that
            focus: !Kirigami.InputMethod.willShowOnActive

            visible: window.leftPage && (window.leftPage.searchFor !== null || window.leftPage.hasOwnProperty("search"))

            page: window.leftPage

            onCurrentSearchTextChanged: {
                var curr = window.leftPage;

                if (pageStack.depth > 1) {
                    pageStack.pop()
                }

                if (currentSearchText === "" && window.currentTopLevel === "" && !window.leftPage.category) {
                    Navigation.openHome()
                } else if (!curr.hasOwnProperty("search")) {
                    if (currentSearchText) {
                        Navigation.clearStack()
                        Navigation.openApplicationList({ search: currentSearchText })
                    }
                } else {
                    curr.search = currentSearchText;
                    curr.forceActiveFocus()
                }
            }
        }
    }

    topContent: [
        ActionListItem {
            action: featuredAction
        },
        ActionListItem {
            action: searchAction
        },
        ActionListItem {
            action: installedAction
            visible: drawer.wideScreen
        },
        ActionListItem {
            objectName: "updateButton"
            action: updateAction
            visible: enabled && drawer.wideScreen

            badgeIconName: ResourcesModel.updatesCount > 0 ? "emblem-important" : ""

            // Disable down navigation on the last item so we don't escape the
            // actual list.
            Keys.onDownPressed: event.accepted = true
        },
        ActionListItem {
            action: sourcesAction
        },
        ActionListItem {
            action: aboutAction
        },
        Kirigami.Separator {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.smallSpacing
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing
        }
    ]

    ColumnLayout {
        spacing: 0
        Layout.fillWidth: true

        Kirigami.Separator {
            visible: progressView.visible
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.smallSpacing
        }

        ProgressView {
            id: progressView
            separatorVisible: false
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
            icon.name: category ? category.icon : ""
            checked: itsMe
            visible: (!window.leftPage
                   || !window.leftPage.subcategories
                   || window.leftPage.subcategories === undefined
                   || currentSearchText.length === 0
                   || (category && category.contains(window.leftPage.subcategories))
                     )
            onTriggered: {
                if (!window.leftPage.canNavigate) {
                    Navigation.openCategory(category, currentSearchText)
                } else {
                    if (pageStack.depth > 1) {
                        pageStack.pop()
                    }
                    pageStack.currentIndex = 0
                    window.leftPage.category = category
                }

                if (!drawer.wideScreen && category.subcategories.length === 0) {
                    drawer.close();
                }
            }
        }
    }
}
