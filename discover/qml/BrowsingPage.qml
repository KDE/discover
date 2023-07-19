/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.15
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.1
import QtQml.Models 2.15
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.19 as Kirigami

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

            inlineMessage: ResourcesModel.inlineMessage
        }
    }

    readonly property bool isHome: true

    function searchFor(text) {
        if (text.length === 0) {
            return;
        }
        Navigation.openCategory(null, "")
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
            readonly property var helpfulError: featuredModel.currentApplicationBackend.explainDysfunction()
            icon.name: helpfulError.iconName
            text: i18n("Unable to load applications")
            explanation: helpfulError.message

            Repeater {
                model: helpfulError.actions
                delegate: Button {
                    Layout.alignment: Qt.AlignHCenter
                    action: ConvertDiscoverAction {
                        action: modelData
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
            model: PaginateModel {
                pageSize: apps.maximumColumns * 2
                sourceModel: OdrsAppsModel {
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
            text: i18nc("@title:group", "Editor's Choice")
            visible: !featuredModel.isFetching
        }

        Repeater {
            model: FeaturedModel {
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
            model: PaginateModel {
                pageSize: apps.maximumColumns
                sourceModel: ResourcesProxyModel {
                    filteredCategoryName: "Games"
                    backendFilter: ResourcesModel.currentApplicationBackend
                    sortRole: ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(CategoryModel.findCategoryByName("Games"))
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
            model: PaginateModel {
                pageSize: apps.maximumColumns
                sourceModel: ResourcesProxyModel {
                    filteredCategoryName: "Developer Tools"
                    backendFilter: ResourcesModel.currentApplicationBackend
                    sortRole: ResourcesProxyModel.SortableRatingRole
                    sortOrder: Qt.DescendingOrder
                }
            }
            delegate: GridApplicationDelegate { visible: !featuredModel.isFetching }
        }

        Button {
            text: i18nc("@action:button", "See More")
            icon.name: "go-next-view"
            Layout.columnSpan: apps.columns
            onClicked: Navigation.openCategory(CategoryModel.findCategoryByName("Developer Tools"))
            visible: devRep.count > 0 && !featuredModel.isFetching
        }
    }
}
