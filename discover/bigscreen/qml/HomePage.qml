// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

BigscreenUI.Page {
    id: root

    // Currently focused app
    readonly property Discover.AbstractResource spotlight: rails.currentItem?.currentApplication ?? featuredRail.currentApplication ?? popularRail.currentApplication ?? null

    readonly property bool loading: featured.isFetching
    readonly property bool empty: !loading && featured.count === 0 && popularRail.count === 0

    background: AppBackdrop {
        icon: root.spotlight?.icon
    }

    BigscreenUI.BusyIndicator {
        anchors.centerIn: parent
        visible: root.loading
        running: visible
    }

    BigscreenUI.PlaceholderMessage {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: root.contentMargins.left
        anchors.rightMargin: root.contentMargins.right
        visible: root.empty
        focus: root.empty

        readonly property Discover.InlineMessage dysfunction: featured.currentApplicationBackend?.explainDysfunction() ?? null

        iconName: dysfunction?.iconName ?? "plasmadiscover"
        text: i18n("Unable to load apps")
        explanation: dysfunction?.message ?? i18n("Check your internet connection, or the sources in settings.")
    }

    BigscreenUI.RailView {
        id: rails

        anchors.fill: parent
        anchors.topMargin: root.contentMargins.top - topMargin
        bottomMargin: root.contentMargins.bottom
        visible: !root.loading && !root.empty
        focus: !root.empty

        // Rails show up as they load, stay on the first one until the user moves
        property bool moved: false
        onCountChanged: {
            if (!moved) {
                Qt.callLater(() => {
                    if (!rails.moved) {
                        rails.currentIndex = 0;
                    }
                });
            }
        }
        Keys.onUpPressed: event => {
            moved = true;
            event.accepted = false;
        }
        Keys.onDownPressed: event => {
            moved = true;
            event.accepted = false;
        }

        AppRail {
            id: featuredRail
            title: i18nc("@title", "Featured")
            model: DiscoverApp.FeaturedModel {
                id: featured
            }
        }

        AppRail {
            id: popularRail
            title: i18nc("@title", "Most popular")
            model: DiscoverApp.LimitedRowCountProxyModel {
                pageSize: 20
                sourceModel: DiscoverApp.OdrsAppsModel {}
            }
        }

        CategoryRail {
            title: i18nc("@title", "Watch")
            categoryName: "Video Players"
        }

        CategoryRail {
            title: i18nc("@title", "Listen")
            categoryName: "Audio Players"
        }

        CategoryRail {
            title: i18nc("@title", "Games")
            categoryName: "Games"
        }

        CategoryRail {
            title: i18nc("@title", "Emulators")
            categoryName: "Emulators"
        }

        CategoryRail {
            title: i18nc("@title", "Recently updated")
            categoryName: "All Applications"
            sortRole: Discover.ResourcesProxyModel.ReleaseDateRole
        }
    }

    component CategoryRail: AppRail {
        id: categoryRail

        required property string categoryName
        property int sortRole: Discover.ResourcesProxyModel.SortableRatingRole

        model: DiscoverApp.LimitedRowCountProxyModel {
            pageSize: 20
            sourceModel: Discover.ResourcesProxyModel {
                filteredCategoryName: categoryRail.categoryName
                backendFilter: Discover.ResourcesModel.currentApplicationBackend
                sortRole: categoryRail.sortRole
                sortOrder: Qt.DescendingOrder
            }
        }
    }
}
