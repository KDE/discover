/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

DiscoverPage {
    id: page

    readonly property var model: appsModel

    property alias category: appsModel.filteredCategory
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias extending: appsModel.extending
    property alias search: appsModel.search
    property alias resourcesUrl: appsModel.resourcesUrl
    property alias isBusy: appsModel.isBusy
    property alias allBackends: appsModel.allBackends
    property alias count: apps.count
    property alias listHeader: apps.header
    property alias listHeaderPositioning: apps.headerPositioning

    property bool showRating: true
    property bool showSize: false
    property bool searchPage: false

    // sortProperty is a key into DiscoverSettings config object to support
    // subtypes that have separate persistent sorting preferences. Changing
    // sortProperty after the component creation is not supported, however
    // the property which it references on the DiscoverSettings singleton is
    // expected to be observable.
    property string sortProperty: "appsListPageSorting"
    // Subclasses should override it with their values according to
    // sortProperty and discoversettings.kcfg
    property int defaultSortRole: Discover.ResourcesProxyModel.SortableRatingRole

    readonly property int sortRole: DiscoverApp.DiscoverSettings[sortProperty]
    function setSortRole(role: int) {
        DiscoverApp.DiscoverSettings[sortProperty] = role;
    }
    // This property is a non-persistent sort role. Currently used for
    // overriding regular sort role with SearchRelevanceRole during search.
    property int sortRoleTemporaryOverride: -1
    readonly property int effectiveSortRole: sortRoleTemporaryOverride !== -1 ? sortRoleTemporaryOverride : sortRole
    readonly property int effectiveSortOrder: effectiveSortRole === Discover.ResourcesProxyModel.NameRole ? Qt.AscendingOrder : Qt.DescendingOrder

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories

    // This property is used to prevent superfluous property writes during initialization.
    property bool __completed: false

    function stripHtml(input) {
        var regex = /(<([^>]+)>)/ig
        return input.replace(regex, "");
    }

    property string name: category?.name ?? ""

    title: {
        const count = appsModel.count;
        if (search.length > 0) {
            if (count.valid) {
                return i18np("Search: %2 - %3 item", "Search: %2 - %3 items", count.number, stripHtml(search), count.string)
            } else {
                return i18n("Search: %1", stripHtml(search))
            }
        } else if (name.length > 0) {
            if (count.valid) {
                return i18np("%2 - %1 item", "%2 - %1 items", count.number, name)
            } else {
                return name
            }
        } else {
            if (count.valid) {
                return i18np("Search - %1 item", "Search - %1 items", count.number)
            } else {
                return i18n("Search")
            }
        }
    }

    signal clearSearch()

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    supportsRefreshing: true
    onRefreshingChanged: if (refreshing) {
        appsModel.invalidateFilter()
        refreshing = false
    }

    onSearchChanged: {
        const isTemporaryOverride = (search.length > 0);
        if (isTemporaryOverride) {
            sortRoleTemporaryOverride = Discover.ResourcesProxyModel.SearchRelevanceRole;
        } else {
            sortRoleTemporaryOverride = -1;
        }
    }

    onEffectiveSortRoleChanged: __updateCheckedAction()

    Component.onCompleted: {
        __completed = true;
        __updateCheckedAction()
    }

    function __updateCheckedAction() {
        if (!__completed) {
            return;
        }

        let action = null;

        switch (effectiveSortRole) {
        case Discover.ResourcesProxyModel.SearchRelevanceRole:
            action = actionSortByRelevance;
            break;

        case Discover.ResourcesProxyModel.NameRole:
            action = actionSortByName;
            break;

        case Discover.ResourcesProxyModel.SortableRatingRole:
            action = actionSortByRating;
            break;

        case Discover.ResourcesProxyModel.SizeRole:
            action = actionSortBySize;
            break;

        case Discover.ResourcesProxyModel.ReleaseDateRole:
            action = actionSortByReleaseDate;
            break;

        default:
            // Fallback if config is corrupted
            const isTemporaryOverride = (search.length > 0);
            if (isTemporaryOverride) {
                sortRoleTemporaryOverride = Discover.ResourcesProxyModel.SearchRelevanceRole;
            } else {
                setSortRole(defaultSortRole);
                sortRoleTemporaryOverride = -1;
                return;
            }
            break;
        }
        sortGroup.checkedAction = action;
    }

    function __checkedActionChanged() {
        if (!__completed) {
            return;
        }

        let role = -1;

        switch (sortGroup.checkedAction) {
        case actionSortByRelevance:
            role = Discover.ResourcesProxyModel.SearchRelevanceRole;
            break;

        case actionSortByName:
            role = Discover.ResourcesProxyModel.NameRole;
            break;

        case actionSortByRating:
            role = Discover.ResourcesProxyModel.SortableRatingRole;
            break;

        case actionSortBySize:
            role = Discover.ResourcesProxyModel.SizeRole;
            break;

        case actionSortByReleaseDate:
            role = Discover.ResourcesProxyModel.ReleaseDateRole;
            break;

        default:
            return;
        }

        const isTemporaryOverride = (search.length > 0);
        if (isTemporaryOverride) {
            sortRoleTemporaryOverride = role;
        } else {
            // Assign role before resetting temporary override to avoid extra sorting pass using a stale sortRole.
            setSortRole(role);
            sortRoleTemporaryOverride = -1;
        }
    }

    QQC2.ActionGroup {
        id: sortGroup
        exclusive: true

        onCheckedActionChanged: page.__checkedActionChanged()
    }

    actions: [
        Kirigami.Action {
            text: i18n("Sort: %1", sortGroup.checkedAction.text)
            icon.name: "view-sort"
            Kirigami.Action {
                id: actionSortByRelevance
                QQC2.ActionGroup.group: sortGroup
                visible: appsModel.search.length > 0
                text: i18nc("Search results most relevant to the search query", "Relevance")
                icon.name: "file-search-symbolic"
                checkable: true
            }
            Kirigami.Action {
                id: actionSortByName
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Name")
                icon.name: "sort-name"
                checkable: true
            }
            Kirigami.Action {
                id: actionSortByRating
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Rating")
                icon.name: "rating"
                checkable: true
            }
            Kirigami.Action {
                id: actionSortBySize
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Size")
                icon.name: "download"
                checkable: true
            }
            Kirigami.Action {
                id: actionSortByReleaseDate
                QQC2.ActionGroup.group: sortGroup
                text: i18n("Release Date")
                icon.name: "change-date-symbolic"
                checkable: true
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

        model: Discover.ResourcesProxyModel {
            id: appsModel

            sortRole: page.effectiveSortRole
            sortOrder: page.effectiveSortOrder

            onBusyChanged: isBusy => {
                if (isBusy) {
                    apps.currentIndex = -1
                }
            }
        }

        delegate: ApplicationDelegate {
            // simply `required application` does not work due to QTBUG-86897
            required property var model

            application: model.application
            compact: !applicationWindow().wideScreen
            showRating: page.showRating
            showSize: page.showSize
        }

        Item {
            readonly property bool nothingFound: apps.count == 0 && !appsModel.isBusy && !Discover.ResourcesModel.isInitializing && (!page.searchPage || appsModel.search.length > 0)

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

                visible: appsModel.search.length > 0 && stateFilter !== Discover.AbstractResource.Installed

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
