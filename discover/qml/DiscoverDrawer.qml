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
import org.kde.kirigami 2.11 as Kirigami
import "navigation.js" as Navigation

Kirigami.GlobalDrawer {
    id: drawer

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    // FIXME: Dirty workaround for 385992
    width: Kirigami.Units.gridUnit * 14

    property bool wideScreen: false

    resetMenuOnTriggered: false

    property string currentSearchText

    onCurrentSubMenuChanged: {
        if (currentSubMenu)
            currentSubMenu.trigger()
        else if (currentSearchText.length > 0)
            window.leftPage.category = null
        else
            Navigation.openHome()
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
        visible: drawer.wideScreen

        contentItem: RowLayout {
            anchors {
                left: parent.left
                leftMargin: Kirigami.Units.smallSpacing
                right: parent.right
                rightMargin: Kirigami.Units.smallSpacing
            }

            SearchField {
                id: searchField
                Layout.fillWidth: true

                visible: window.leftPage && (window.leftPage.searchFor !== null || window.leftPage.hasOwnProperty("search"))

                page: window.leftPage

                onCurrentSearchTextChanged: {
                    var curr = window.leftPage;

                    if (pageStack.depth>1)
                        pageStack.pop()

                    if (currentSearchText === "" && window.currentTopLevel === "" && !window.leftPage.category) {
                        Navigation.openHome()
                    } else if (!curr.hasOwnProperty("search")) {
                        if (currentSearchText) {
                            Navigation.clearStack()
                            Navigation.openApplicationList( { search: currentSearchText })
                        }
                    } else {
                        curr.search = currentSearchText;
                        curr.forceActiveFocus()
                    }
                }
            }

            ToolButton {

                icon.name: "go-home"
                onClicked: Navigation.openHome()

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
            action: searchAction
        }
        ActionListItem {
            action: installedAction
        }
        ActionListItem {
            action: sourcesAction
        }

        ActionListItem {
            action: aboutAction
        }

        ActionListItem {
            objectName: "updateButton"
            action: updateAction

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
            onTriggered: {
                if (!window.leftPage.canNavigate)
                    Navigation.openCategory(category, currentSearchText)
                else {
                    if (pageStack.depth>1)
                        pageStack.pop()
                    pageStack.currentIndex = 0
                    window.leftPage.category = category
                }
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
