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

    header: Item {
        height: !message.active ? 0 : message.height + message.anchors.margins

        DiscoverInlineMessage {
            id: message

            anchors {
                top: parent.top
                left: parent.left
                right: parent.right
                margins: Kirigami.Units.smallSpacing
            }

            inlineMessage: Discover.ResourcesModel.inlineMessage
        }
    }

    readonly property bool isHome: true

    Kirigami.Theme.colorSet: Kirigami.Theme.Window
    Kirigami.Theme.inherit: false

    Kirigami.LoadingPlaceholder {
        visible: featuredModel.isFetching
        anchors.centerIn: parent
    }

    Loader {
        active: featuredModel.count === 0 && !featuredModel.isFetching
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)
        sourceComponent: Kirigami.PlaceholderMessage {
            readonly property Discover.InlineMessage helpfulError: featuredModel.currentApplicationBackend.explainDysfunction()

            icon.name: helpfulError.iconName
            text: i18n("Unable to load applications")
            explanation: helpfulError.message

            Repeater {
                model: helpfulError.actions
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

    footer: ColumnLayout {
        spacing: 0

        Kirigami.Separator {
            Layout.fillWidth: true
            visible: Kirigami.Settings.isMobile && inlineMessage.visible
        }
    }

    Kirigami.CardsLayout {
        id: apps

        maximumColumns: 4
        rowSpacing: Kirigami.Units.gridUnit
        columnSpacing: Kirigami.Units.gridUnit
        maximumColumnWidth: Kirigami.Units.gridUnit * 6

        Kirigami.Heading {
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            text: i18nc("@title:group", "Most Popular")
            visible: popRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: popRep
            model: DiscoverApp.PaginateModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: DiscoverApp.OdrsAppsModel {
                    // filter: FOSS
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.gridUnit
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            text: i18nc("@title:group", "Newly Published & Recently Updated")
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
                // TODO: Add packagekit-backend of rolling distros
                return [
                    "flatpak-backend",
                    "snap-backend",
                ].includes(Discover.ResourcesModel.currentApplicationBackend.name);
            }

            DiscoverApp.PaginateModel {
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
            Layout.topMargin: Kirigami.Units.gridUnit
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            text: i18nc("@title:group", "Editor's Choice")
            visible: featuredRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: featuredRep
            model: DiscoverApp.FeaturedModel {
                id: featuredModel
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.gridUnit
            // Need to undo some the row spacing of the parent layout which looks bad here
            Layout.bottomMargin: -(apps.rowSpacing / 2)
            Layout.columnSpan: apps.columns
            text: i18nc("@title:group", "Highest-Rated Games")
            visible: gamesRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: gamesRep
            model: DiscoverApp.PaginateModel {
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
        }

        Kirigami.Heading {
            Layout.topMargin: Kirigami.Units.gridUnit
            Layout.columnSpan: apps.columns
            text: i18nc("@title:group", "Highest-Rated Developer Tools")
            visible: devRep.count > 0 && !featuredModel.isFetching
        }

        Repeater {
            id: devRep
            model: DiscoverApp.PaginateModel {
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
        }
    }
}
