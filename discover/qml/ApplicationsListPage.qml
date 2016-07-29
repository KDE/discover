/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.5
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import "navigation.js" as Navigation
import org.kde.discover.app 1.0
import org.kde.discover 1.0
import org.kde.kirigami 1.0 as Kirigami

DiscoverPage {
    id: page
    readonly property var model: appsModel
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.stringSortRole
    property alias sortOrder: appsModel.sortOrder
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias extend: appsModel.extends
    property alias search: appsModel.search
    property alias shouldShowTechnical: appsModel.isShowingTechnical
    property alias header: apps.header
    title: category ? category.name : ""

    onSearchChanged: appsModel.sortOrder = Qt.AscendingOrder
    signal clearSearch()

    ListView {
        id: apps
        anchors.fill: parent
        section.delegate: Label {
            text: section
            anchors {
                right: parent.right
            }
        }

        header: CategoryDisplay {
            category: appsModel.filteredCategory
            subcategories: appsModel.subcategories
            search: appsModel.search
            anchors {
                left: parent.left
                right: parent.right
            }

            Item {
                Layout.fillWidth: true
                height: Kirigami.Units.largeSpacing * 3
            }
            z: 5000
        }
        model: ApplicationProxyModel {
            id: appsModel
            isSortingByRelevancy: true
            stringSortRole: "ratingPoints"
            sortOrder: Qt.DescendingOrder
            isShowingTechnical: category && category.shouldShowTechnical

            Component.onCompleted: sortModel()
        }
        spacing: Kirigami.Units.gridUnit
        delegate: ApplicationDelegate {
            x: Kirigami.Units.gridUnit
            width: ListView.view.width - Kirigami.Units.gridUnit*2
        }
    }
}
