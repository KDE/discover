/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.5
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import "navigation.js" as Navigation
import org.kde.discover.app 1.0
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

DiscoverPage {
    id: page
    readonly property var model: appsModel
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.sortRole
    property alias sortOrder: appsModel.sortOrder
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias extending: appsModel.extending
    property alias search: appsModel.search
    property alias resourcesUrl: appsModel.resourcesUrl
    property alias isBusy: appsModel.isBusy
    property alias allBackends: appsModel.allBackends
    property alias roughCount: appsModel.roughCount
    property alias count: apps.count
    property alias listHeader: apps.header
    property alias listHeaderPositioning: apps.headerPositioning
    property string sortProperty: "appsListPageSorting"
    property bool compact: page.width < Kirigami.Units.gridUnit * 28 || !applicationWindow().wideScreen
    property bool showRating: true
    property bool showSize: false
    property bool searchPage: false

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories

    function stripHtml(input) {
        var regex = /(<([^>]+)>)/ig
        return input.replace(regex, "");
    }

    property string name: category ? category.name : ""
    title: {
        const rough = appsModel.roughCount;
        if (search.length>0) {
            if (rough.length > 0) {
                return i18np("Search: %2 - %1 item", "Search: %2 - %1 items", rough, stripHtml(search))
            } else {
                return i18n("Search: %1", stripHtml(search))
            }
        } else if (name.length > 0) {
            if (rough.length > 0) {
                return i18np("%2 - %1 item", "%2 - %1 items", rough, name)
            } else {
                return name
            }
        } else {
            if (rough.length > 0) {
                return i18np("Search - %1 item", "Search - %1 items", rough)
            } else {
                return i18n("Search")
            }
        }
    }

    signal clearSearch()

    supportsRefreshing: true
    onRefreshingChanged: if (refreshing) {
        appsModel.invalidateFilter()
        refreshing = false
    }

    ActionGroup {
        id: sortGroup
        exclusive: true
    }

    contextualActions: [
        Kirigami.Action {
            visible: !appsModel.sortByRelevancy
            text: i18n("Sort: %1", sortGroup.checkedAction.text)
            Action {
                ActionGroup.group: sortGroup
                text: i18n("Name")
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.NameRole
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.NameRole
            }
            Action {
                ActionGroup.group: sortGroup
                text: i18n("Rating")
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.SortableRatingRole
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.SortableRatingRole
            }
            Action {
                ActionGroup.group: sortGroup
                text: i18n("Size")
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.SizeRole
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.SizeRole
            }
            Action {
                ActionGroup.group: sortGroup
                text: i18n("Release Date")
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.ReleaseDateRole
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.ReleaseDateRole
            }
        }
    ]

    Kirigami.CardsListView {
        id: apps
        activeFocusOnTab: true
        currentIndex: -1
        onActiveFocusChanged: if (activeFocus && currentIndex === -1) {
            currentIndex = 0;
        }

        section.delegate: Label {
            text: section
            anchors {
                right: parent.right
            }
        }

        model: ResourcesProxyModel {
            id: appsModel
            sortRole: DiscoverSettings.appsListPageSorting
            sortOrder: sortRole === ResourcesProxyModel.NameRole ? Qt.AscendingOrder : Qt.DescendingOrder

            onBusyChanged: if (isBusy) {
                apps.currentIndex = -1
            }
        }
        delegate: ApplicationDelegate {
            application: model.application
            compact: !applicationWindow().wideScreen
            showRating: page.showRating
            showSize: page.showSize
        }

        Item {
            readonly property bool nothingFound: apps.count == 0 && !appsModel.isBusy && !ResourcesModel.isInitializing && (!page.searchPage || appsModel.search.length > 0)

            anchors.fill: parent
            opacity: nothingFound ? 1 : 0
            visible: opacity > 0
            Behavior on opacity { NumberAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad } }

            Kirigami.PlaceholderMessage {
                visible: !searchedForThingNotFound.visible
                anchors.centerIn: visible ? parent : undefined
                width: parent.width - (Kirigami.Units.largeSpacing * 8)

                icon.name: "edit-none"
                text: i18n("Nothing found")
            }

            Kirigami.PlaceholderMessage {
                id: searchedForThingNotFound

                property var searchAllCategoriesAction: Kirigami.Action {
                    text: i18nc("@action:button", "Search in All Categories")
                    icon.name: "search"
                    onTriggered: {
                        window.globalDrawer.resetMenu();
                        Navigation.clearStack()
                        Navigation.openApplicationList( { search: page.search } );
                    }
                }
                property var searchTheWebAction: Kirigami.Action {
                    text: i18nc("@action:button %1 is the name of an application", "Search the Web for \"%1\"", appsModel.search)
                    icon.name: "internet-web-browser"
                    onTriggered: {
                        const searchTerm = encodeURIComponent("Linux " + appsModel.search);
                        Qt.openUrlExternally(i18nc("If appropriate, localize this URL to be something more relevant to the language. %1 is the text that will be searched for.", "https://duckduckgo.com/?q=%1", searchTerm));
                    }
                }

                anchors.centerIn: parent
                width: parent.width - (Kirigami.Units.largeSpacing * 8)

                visible: appsModel.search.length > 0 && stateFilter !== AbstractResource.Installed

                icon.name: "edit-none"
                text: page.category ? i18nc("@info:placeholder %1 is the name of an application; %2 is the name of a category of apps or add-ons",
                                            "\"%1\" was not found in the \"%2\" category", appsModel.search, page.category.name)
                                    : i18nc("@info:placeholder %1 is the name of an application",
                                            "\"%1\" was not found in the available sources", appsModel.search)
                explanation: page.category ? "" : i18nc("@info:placeholder%1 is the name of an application", "\"%1\" may be available on the web. Software acquired from the web has not been reviewed by your distributor for functionality or stability. Use with caution.", appsModel.search)

                // If we're in a category, first direct the user to search globally,
                // because they might not have realized they were in a category and
                // therefore the results were limited to just what was in the category
                helpfulAction: page.category ? searchAllCategoriesAction : searchTheWebAction
            }
        }

        Kirigami.PlaceholderMessage {
            anchors.centerIn: parent
            width: parent.width - (Kirigami.Units.largeSpacing * 4)

            visible: opacity !== 0
            opacity: apps.count == 0 && page.searchPage && appsModel.search.length == 0 ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad } }

            icon.name: "search"
            text: i18n("Search")
        }

        footer: ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: appsModel.isBusy && apps.atYEnd
            opacity: visible ? 0.5 : 0
            height: visible ? Layout.preferredHeight : 0

            Item {
                Layout.preferredHeight: Kirigami.Units.gridUnit
            }
            Kirigami.Heading {
                level: 2
                Layout.alignment: Qt.AlignCenter
                text: i18n("Still looking…")
            }
            BusyIndicator {
                running: parent.visible
                Layout.alignment: Qt.AlignCenter
                Layout.preferredWidth: Kirigami.Units.gridUnit * 4
                Layout.preferredHeight: Kirigami.Units.gridUnit * 4
            }
            Behavior on opacity {
                PropertyAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad }
            }
        }
    }
}
