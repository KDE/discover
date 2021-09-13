/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.15 as QQC2
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.15 as Kirigami
import Qt.labs.qmlmodels 1.0

import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Kirigami.OverlayDrawer {
    id: sidebar

    edge: Qt.application.layoutDirection === Qt.RightToLeft ? Qt.RightEdge : Qt.LeftEdge
    modal: !wideScreen
    onModalChanged: drawerOpen = !modal
    handleVisible: !wideScreen
    handleClosedIcon.source: null
    handleOpenIcon.source: null
    drawerOpen: !Kirigami.Settings.isMobile
    width: Kirigami.Units.gridUnit * 16

    Kirigami.Theme.colorSet: Kirigami.Theme.Window

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    property string currentSearchText: searchField.text

    Component.onCompleted: {
        if (searchField.visible)
            searchField.forceActiveFocus();
    }

    function suggestSearchText(text) {
        if (searchField.visible) {
            searchField.text = text
            searchField.forceActiveFocus()
        }
    }

    contentItem: ColumnLayout {
        id: container

        QQC2.ToolBar {
            id: toolbar
            Layout.fillWidth: true
            Layout.preferredHeight: pageStack.globalToolBar.preferredHeight

            leftPadding: Kirigami.Units.smallSpacing
            rightPadding: Kirigami.Units.smallSpacing
            topPadding: 0
            bottomPadding: 0

            Kirigami.SearchField {
                id: searchField
                Layout.fillWidth: true
                anchors {
                    left: parent.left
                    leftMargin: Kirigami.Units.smallSpacing
                    right: parent.right
                    rightMargin: Kirigami.Units.smallSpacing
                    verticalCenter: parent.verticalCenter
                }
            }
        }

        QQC2.ScrollView {
            id: generalView
            implicitWidth: Kirigami.Units.gridUnit * 16
            Layout.fillWidth: true
            Layout.topMargin: toolbar.visible ? -Kirigami.Units.smallSpacing - 1 : 0
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            contentWidth: availableWidth

            clip: true

            ListView {
                id: generalList
                currentIndex: 0
                property list<Kirigami.Action> actions: [
                    Kirigami.Action {
                        text: i18n("Home")
                        icon.name: "go-home"
                        property string component: topBrowsingComp
                        checked: window.currentTopLevel==component

                        onTriggered: {
                            if(window.currentTopLevel!=component)
                                window.currentTopLevel=component
                        }
                    },
                    Kirigami.Action {
                        text: i18n("Installed")
                        icon.name: "view-list-details"
                        property string component: topInstalledComp
                        checked: window.currentTopLevel==component

                        onTriggered: {
                            if(window.currentTopLevel!=component)
                                window.currentTopLevel=component
                        }
                    },
                    Kirigami.Action {
                        text: ResourcesModel.updatesCount<=0 ? (ResourcesModel.isFetching ? i18n("Fetching updates…") : i18n("Up to date") ) : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
                        icon.name: updateAction.icon.name
                        property bool hasUpdates: ResourcesModel.updatesCount > 0

                        property string component: topUpdateComp
                        checked: window.currentTopLevel==component

                        onTriggered: {
                            if(window.currentTopLevel!=component)
                                window.currentTopLevel=component
                        }
                    },
                    Kirigami.Action {
                        text: i18n("Settings")
                        icon.name: "settings-configure"
                        shortcut: StandardKey.Preferences
                        property string component: topSourcesComp
                        checked: window.currentTopLevel==component

                        onTriggered: {
                            if(window.currentTopLevel!=component)
                                window.currentTopLevel=component
                        }
                    },
                    Kirigami.Action {
                        text: i18n("About")
                        icon.name: "help-about"
                        property string component: topAboutComp
                        checked: window.currentTopLevel==component

                        onTriggered: {
                            if(window.currentTopLevel!=component)
                                window.currentTopLevel=component
                        }
                    }
                ]
                model: actions
                delegate: Kirigami.BasicListItem {
                    label: modelData.text
                    icon: modelData.icon.name
                    separatorVisible: false
                    action: modelData
                    backgroundColor: hasUpdates ? "orange" : Kirigami.Theme.viewBackgroundColor
                    onClicked: categoriesList.currentIndex = -1;
                }
            }
        }

        QQC2.ScrollView {
            id: categoriesView
            implicitWidth: Kirigami.Units.gridUnit * 16
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Kirigami.Units.largeSpacing * 2
            QQC2.ScrollBar.horizontal.policy: QQC2.ScrollBar.AlwaysOff
            contentWidth: availableWidth

            clip: true

            ListView {
                id: categoriesList

                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing

                header: Kirigami.Heading {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    leftPadding: Kirigami.Units.largeSpacing
                    bottomPadding: Kirigami.Units.smallSpacing
                    text: i18n("Categories")
                    color: Kirigami.Theme.disabledTextColor
                    level: 4
                    z: 10
                    background: Rectangle {color: Kirigami.Theme.backgroundColor}
                }
                headerPositioning: ListView.OverlayHeader

                currentIndex: -1

                model: CategoryModel.rootCategories[0].subcategories

                delegate: Kirigami.BasicListItem {
                    label: modelData.name
                    separatorVisible: false
                    trailing: Kirigami.Icon {
                        width: Kirigami.Units.iconSizes.small
                        height: Kirigami.Units.iconSizes.small
                        source: CategoryModel.rootCategories[0].subcategories[index].subcategories.length > 0 ? 'arrow-right' : ''
                    }
                    onClicked: {
                        onClicked: generalList.currentIndex = -1;
                        if (CategoryModel.rootCategories[0].subcategories[index].subcategories.length > 0) {
                            let category = CategoryModel.rootCategories[0].subcategories[index];
                            Navigation.openCategory(category, currentSearchText);
                        } else {
                            let category = CategoryModel.rootCategories[0].subcategories[index];
                            Navigation.openCategory(category, currentSearchText);
                        }
                    }
                }
            }
        }
    }
}

/*
Kirigami.GlobalDrawer {
    id: drawer

    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    FIXME: Dirty workaround for 385992
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
        if (searchField.visible) {
            searchField.text = text
            searchField.forceActiveFocus()
        }
    }

    Give the search field keyboard focus by default
    Component.onCompleted: {
        if (searchField.visible)
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

        Kirigami.Separator {
            Layout.fillWidth: true
        }

        ProgressView {
            separatorVisible: false
        }

        ActionListItem {
            action: featuredAction
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
*/
