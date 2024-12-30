/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

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

    function createCategoryActions(categories /*list<Discover.Category>*/) /*list<Kirigami.Action>*/ {
        const ret = []
        for (const category of categories) {
            const categoryAction = categoryActionComponent.createObject(drawer, { category })
            categoryAction.children = createCategoryActions(category.subcategories)
            ret.push(categoryAction)
        }
        return ret;
    }
    actions: createCategoryActions(Discover.CategoryModel.rootCategories)

    padding: 0
    topPadding: undefined
    leftPadding: undefined
    rightPadding: undefined
    bottomPadding: undefined
    verticalPadding: undefined
    horizontalPadding: undefined

    // FIXME: Dirty workaround for 385992
    width: Kirigami.Units.gridUnit * 14

    resetMenuOnTriggered: false
    modal: !drawer.wideScreen

    onCurrentSubMenuChanged: {
        if (currentSubMenu) {
            currentSubMenu.trigger()
        } else if (currentSearchText.length > 0) {
            window.leftPage.category = null
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
                drawer.currentSearchText = currentSearchText
            }

            Keys.onDownPressed: featuredActionListItem.forceActiveFocus(Qt.TabFocusReason)
        }
    }

    topContent: [
        ActionListItem {
            id: featuredActionListItem
            action: featuredAction
            Keys.onUpPressed: searchField.forceActiveFocus(Qt.TabFocusReason)
        },
        ActionListItem {
            action: searchAction
        },
        ActionListItem {
            action: installedAction
            visible: enabled && drawer.wideScreen
        },
        ActionListItem {
            objectName: "updateButton"
            action: updateAction
            visible: enabled && drawer.wideScreen
            stateIconName: Discover.ResourcesModel.updatesCount > 0 ? "emblem-important" : ""
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

    footer: ColumnLayout {
        spacing: 0
        Layout.fillWidth: true

        Kirigami.Separator {
            visible: progressView.visible
            Layout.fillWidth: true
        }

        ProgressView {
            id: progressView
            Layout.fillWidth: true
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
            required property Discover.Category category

            readonly property bool itsMe: window?.leftPage?.category === category

            text: category?.name ?? ""
            icon.name: category?.icon + "-symbolic" ?? ""
            checked: itsMe
            enabled: (currentSearchText.length === 0
                      || (category?.contains(window?.leftPage?.model?.subcategories) ?? false))

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
