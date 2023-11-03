/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtQuick.Window
import "navigation.js" as Navigation
import org.kde.discover.app
import org.kde.discover
import org.kde.kirigami 2 as Kirigami

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
        if (search.length > 0) {
            if (rough.length > 0) {
                return i18np("Search: %2 - %3 item", "Search: %2 - %3 items", appsModel.count, stripHtml(search), rough)
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

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    onSearchChanged: {
        if (search.length > 0) {
            appsModel.tempSortRole = ResourcesProxyModel.SearchRelevanceRole
        } else {
            appsModel.tempSortRole = -1
        }
    }

    supportsRefreshing: true
    onRefreshingChanged: if (refreshing) {
        appsModel.invalidateFilter()
        refreshing = false
    }

    QQC2.ActionGroup {
        id: sortGroup
        exclusive: true
    }

    actions: [
        Kirigami.Action {
            text: i18n("Sort: %1", sortGroup.checkedAction.text)
            icon.name: "view-sort"
            Kirigami.Action {
                visible: appsModel.search.length > 0
                QQC2.ActionGroup.group: sortGroup
                text: i18nc("Search results most relevant to the search query", "Relevance")
                icon.name: "file-search-symbolic"
                onTriggered: {
                    // Do *not* save the sort role on searches
                    appsModel.tempSortRole = ResourcesProxyModel.SearchRelevanceRole
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.SearchRelevanceRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Name")
                icon.name: "sort-name"
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.NameRole
                    appsModel.tempSortRole = -1
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.NameRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Rating")
                icon.name: "rating"
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.SortableRatingRole
                    appsModel.tempSortRole = -1
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.SortableRatingRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Size")
                icon.name: "download"
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.SizeRole
                    appsModel.tempSortRole = -1
                }
                checkable: true
                checked: appsModel.sortRole === ResourcesProxyModel.SizeRole
            }
            Kirigami.Action {
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Release Date")
                icon.name: "change-date-symbolic"
                onTriggered: {
                    DiscoverSettings[page.sortProperty] = ResourcesProxyModel.ReleaseDateRole
                    appsModel.tempSortRole = -1
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

        section.delegate: QQC2.Label {
            text: section
            anchors {
                right: parent.right
            }
        }

        model: ResourcesProxyModel {
            id: appsModel
            property int tempSortRole: -1
            sortRole: tempSortRole >= 0 ? tempSortRole : DiscoverSettings.appsListPageSorting
            sortOrder: sortRole === ResourcesProxyModel.NameRole ? Qt.AscendingOrder : Qt.DescendingOrder

            onBusyChanged: isBusy => {
                if (isBusy) {
                    apps.currentIndex = -1
                }
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

                Kirigami.Action {
                    id: searchAllCategoriesAction
                    text: i18nc("@action:button", "Search in All Categories")
                    icon.name: "search"
                    onTriggered: {
                        window.globalDrawer.resetMenu();
                        Navigation.clearStack()
                        Navigation.openApplicationList({ search: page.search });
                    }
                }
                Kirigami.Action {
                    id: searchTheWebAction
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
                explanation: page.category ? "" : i18nc("@info:placeholder %1 is the name of an application", "\"%1\" may be available on the web. Software acquired from the web has not been reviewed by your distributor for functionality or stability. Use with caution.", appsModel.search)

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
            opacity: apps.count === 0 && page.searchPage && appsModel.search.length === 0 ? 1 : 0
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
            QQC2.BusyIndicator {
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
