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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import "navigation.js" as Navigation
import org.kde.discover.app 1.0
import org.kde.discover 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ScrollablePage {
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
    property Component sectionDelegate: null
    property alias header: apps.header
    property Component extendedToolBar: null
    title: category ? category.name : ""

    onSearchChanged: appsModel.sortOrder = Qt.AscendingOrder

    function changeSorting(role, sorting, section) {
        appsModel.stringSortRole = role
        appsModel.sortOrder=sorting
        apps.section.property = section
        apps.section.delegate = role=="canUpgrade" ? installedSectionDelegate : defaultSectionDelegate
    }

    readonly property var fu: ExclusiveGroup { id: sortActionGroup }
    readonly property string currentSortAction: sortActionGroup.current.text
    readonly property Menu sortMenu: Menu {
        MenuItem {
            text: i18n("Name")
            onTriggered: page.changeSorting("name", Qt.AscendingOrder, "")
            checked: appsModel.stringSortRole=="name"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
        MenuItem {
            text: i18n("Popularity")
            onTriggered: page.changeSorting("sortableRating", Qt.DescendingOrder, "")
            checked: appsModel.stringSortRole=="sortableRating"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
        MenuItem {
            text: i18n("Buzz")
            onTriggered: page.changeSorting("ratingPoints", Qt.DescendingOrder, "")
            checked: appsModel.stringSortRole=="ratingPoints"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
        MenuItem {
            text: i18n("Origin")
            onTriggered: page.changeSorting("origin", Qt.DescendingOrder, "origin")
            checked: appsModel.stringSortRole=="origin"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
        MenuItem {
            text: i18n("Installed")
            onTriggered: page.changeSorting("canUpgrade", Qt.DescendingOrder, "canUpgrade")
            checked: appsModel.stringSortRole=="canUpgrade"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
        MenuItem {
            text: i18n("Size")
            onTriggered: page.changeSorting("size", Qt.DescendingOrder, "")
            checked: appsModel.stringSortRole=="size"
            checkable: true
            exclusiveGroup: sortActionGroup
        }
    }

    Component {
        id: categoryHeaderComponent
        CategoryDisplay {
            id: categoryHeader
            category: page.category
            search: appsModel.search
            width: apps.width

            Item {
                Layout.fillWidth: true
                height: Kirigami.Units.largeSpacing * 3
            }
        }
    }

    Component {
        id: defaultSectionDelegate
        Label {
            text: section
            anchors {
                right: parent.right
            }
        }
    }
    
    Component {
        id: installedSectionDelegate
        Label {
            text: (section=="true" ? i18n("Update") :
                section=="false" ? i18n("Installed") :
                section)
            anchors {
                right: parent.right
            }
        }
    }
    ListView {
        id: apps
        anchors.fill: parent

        header: categoryHeaderComponent
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
