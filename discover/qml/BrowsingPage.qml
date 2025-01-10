/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
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

    title: i18nc("@title:window the name of a top-level 'home' page", "Home")
    objectName: "featured"

    actions: window.wideScreen ? [ searchAction ] : []

    header: DiscoverInlineMessage {
        id: message

        inlineMessage: Discover.ResourcesModel.inlineMessage ? Discover.ResourcesModel.inlineMessage : app.homePageMessage
    }

    readonly property bool isHome: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    DiscoverApp.FeaturedModel {
        id: featuredModel
    }

    Kirigami.LoadingPlaceholder {
        visible: featuredModel.isFetching
        anchors.centerIn: parent
    }

    Loader {
        active: featuredModel.count === 0 && !featuredModel.isFetching
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        sourceComponent: Kirigami.PlaceholderMessage {
            readonly property Discover.InlineMessage helpfulError: featuredModel.currentApplicationBackend?.explainDysfunction() ?? null

            icon.name: helpfulError?.iconName ?? ""
            text: i18n("Unable to load applications")
            explanation: helpfulError?.message ?? ""

            Repeater {
                model: helpfulError?.actions ?? null
                delegate: QQC2.Button {
                    id: delegate

                    required property Discover.DiscoverAction modelData

                    Layout.alignment: Qt.AlignHCenter

                    action: ConvertDiscoverAction {
                        action: delegate.modelData
                    }
                }
            }
        }
    }

    signal clearSearch()

    Kirigami.CardsLayout {
        id: apps

        maximumColumns: 4
        rowSpacing: page.padding
        columnSpacing: page.padding
        maximumColumnWidth: Kirigami.Units.gridUnit * 6

        Kirigami.Heading {
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            text: i18nc("@title:group", "Most Popular")
            wrapMode: Text.Wrap
            visible: popRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: popRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: DiscoverApp.OdrsAppsModel {
                    // filter: FOSS
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Kirigami.Heading {
            Layout.topMargin: page.padding
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            text: i18nc("@title:group", "Newly Published & Recently Updated")
            wrapMode: Text.Wrap
            visible: recentlyUpdatedRepeater.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: recentlyUpdatedRepeater
            model: recentlyUpdatedModelInstantiator.object
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Instantiator {
            id: recentlyUpdatedModelInstantiator

            active: {
                const backend = Discover.ResourcesModel.currentApplicationBackend;
                if (!backend) {
                    return [];
                }
                // TODO: Add packagekit-backend of rolling distros
                return [
                    "flatpak-backend",
                    "snap-backend",
                ].includes(backend.name);
            }

            DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "All Applications"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.ReleaseDateRole
                    sortOrder: Qt.DescendingOrder
                }
            }
        }

        Kirigami.Heading {
            Layout.topMargin: page.padding
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            text: i18nc("@title:group", "Editor's Choice")
            wrapMode: Text.Wrap
            visible: featuredRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: featuredRep
            model: featuredModel
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Kirigami.Heading {
            Layout.topMargin: page.padding
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            text: i18nc("@title:group", "Highest-Rated Games")
            wrapMode: Text.Wrap
            visible: gamesRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: gamesRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "Games"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        QQC2.Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(Discover.CategoryModel.findCategoryByName("Games"))
            visible: gamesRep.count > 0 && !featuredModel.isFetching
            onFocusChanged: {
                if (focus) {
                    if (y < page.flickable.contentY) {
                        page.flickable.contentY = Math.max(0, Math.round(y + height / 2 - page.flickable.height / 2))
                    } else if ((y + height) > (page.flickable.contentY + page.flickable.height)) {
                        page.flickable.contentY = Math.min(page.flickable.contentHeight - page.flickable.height, y  + Math.round(height / 2 - page.flickable.height / 2))
                    }
                }
            }
        }

        Kirigami.Heading {
            Layout.topMargin: page.padding
            Layout.columnSpan: apps.columns
            Layout.fillWidth: true
            text: i18nc("@title:group", "Highest-Rated Developer Tools")
            wrapMode: Text.Wrap
            visible: devRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: devRep
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: apps.maximumColumns
                sourceModel: Discover.ResourcesProxyModel {
                    filteredCategoryName: "Developer Tools"
                    backendFilter: Discover.ResourcesModel.currentApplicationBackend
                    sortRole: Discover.ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        QQC2.Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(Discover.CategoryModel.findCategoryByName("Developer Tools"))
            visible: devRep.count > 0 && !featuredModel.isFetching
            onFocusChanged: {
                if (focus) {
                    if (y < page.flickable.contentY) {
                        page.flickable.contentY = Math.max(0, y + Math.round(height / 2 - page.flickable.height / 2))
                    } else if ((y + height) > (page.flickable.contentY + page.flickable.height)) {
                        page.flickable.contentY = Math.min(page.flickable.contentHeight - page.flickable.height, y  + Math.round(height / 2 - page.flickable.height / 2))
                    }
                }
            }
        }
    }
}
