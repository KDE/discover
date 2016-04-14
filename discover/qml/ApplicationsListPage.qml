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

Item {
    id: page
    readonly property var model: appsModel
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.stringSortRole
    property alias sortOrder: appsModel.sortOrder
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias search: appsModel.search
    property alias shouldShowTechnical: appsModel.isShowingTechnical
    property string sectionProperty: ""
    property Component sectionDelegate: null
    property bool preferList: false
    readonly property real proposedMargin: (width-Helpers.actualWidth)/2
    property Component header: category==null ? null : categoryHeaderComponent
    property Component extendedToolBar: null
    property var icon: category ? category.icon : "go-home"
    property string title: category ? category.name : ""

    onSearchChanged: appsModel.sortOrder = Qt.AscendingOrder
    
    function searchFor(text) {
        appsModel.search = text
        appsModel.isSortingByRelevancy = true
    }
    
    ApplicationProxyModel {
        id: appsModel
        stringSortRole: "ratingPoints"
        sortOrder: Qt.DescendingOrder
        isShowingTechnical: category && category.shouldShowTechnical
        
        Component.onCompleted: sortModel()
    }

    function changeSorting(role, sorting, section) {
        appsModel.stringSortRole = role
        appsModel.sortOrder=sorting
        page.sectionProperty = section
        page.sectionDelegate = role=="canUpgrade" ? installedSectionDelegate : defaultSectionDelegate
    }

    ExclusiveGroup { id: sortActionGroup }
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
    
    property Component tools: RowLayout {
            visible: page.visible
            spacing: 3

            Loader {
                width: item ? item.width : 0
                sourceComponent: page.extendedToolBar
            }

            ToolButton {
                id: sortButton
                iconName: "view-sort-ascending"
                onClicked: menu.popup()

                menu: sortMenu
            }
            ToolButton {
                id: listViewShown
                iconName: "tools-wizard"
                onClicked: shownMenu.popup()

                ExclusiveGroup { id: shownActionGroup }
                menu: Menu {
                    id: shownMenu
                    MenuItem {
                        id: itemList
                        property string viewType: "list"
                        text: i18n("List")
                        checkable: true
                        checked: page.state==viewType
                        onTriggered: page.state=viewType
                        exclusiveGroup: shownActionGroup
                    }
                    MenuItem {
                        id: itemGrid
                        property string viewType: "grid2"
                        text: i18n("Grid")
                        checkable: true
                        checked: page.state==viewType
                        onTriggered: page.state=viewType
                        exclusiveGroup: shownActionGroup
                    }
                    MenuSeparator {}
                    MenuItem {
                        checkable: true
                        checked: appsModel.shouldShowTechnical
                        onTriggered: {
                            appsModel.shouldShowTechnical = !appsModel.shouldShowTechnical;
                            appsModel.sortModel();
                        }
                        text: i18n("Show technical packages")
                    }
                }
            }
        }
    
    Component {
        id: categoryHeaderComponent
        CategoryDisplay {
            id: categoryHeader
            category: page.category
            height: implicitHeight
            spacing: 10
            maxtopwidth: viewLoader.sourceComponent == listComponent ? 100 : viewLoader.item.cellWidth
            x: viewLoader.sourceComponent == listComponent ? page.proposedMargin : 0
        }
    }
    
    Loader {
        id: viewLoader
        anchors.fill: parent
    }

    Component {
        id: appListHeader
        ColumnLayout {
            Loader { sourceComponent: categoryHeaderComponent }
            Label {
                text: i18n("Resources")
                Layout.fillWidth: true
                font.weight: Font.Bold
                Layout.minimumHeight: paintedHeight*1.5
            }
        }
    }
    
    Component {
        id: listComponent
        ApplicationsList {
            id: apps
            anchors.fill: parent
            section.property: page.sectionProperty
            section.delegate: page.sectionDelegate

            header: page.header == categoryHeaderComponent ? appListHeader : page.header
            model: appsModel
        }
    }
    
    Component {
        id: defaultSectionDelegate
        Label {
            text: section
            anchors {
                right: parent.right
                rightMargin: page.proposedMargin
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
                rightMargin: page.proposedMargin
            }
        }
    }
    
    Component {
        id: gridComponent
        ScrolledAwesomeGrid {
            id: theGrid
            model: appsModel
            header: page.header

            section: RowLayout {
                Label { text: i18n("All") }
                Item { Layout.fillWidth: true }
                Label { text: i18np("%1 item", "%1 items", theGrid.count) }
            }

            delegate: ApplicationsGridDelegate {
                width: theGrid.cellWidth
                height: theGrid.cellWidth/1.618 //tau
            }
        }
    }
    
    state: preferList || Helpers.isCompact ? "list" : "grid2"
    states: [
        State {
            name: "list"
            PropertyChanges { target: viewLoader; sourceComponent: listComponent }
        },
        State {
            name: "grid2"
            PropertyChanges { target: viewLoader; sourceComponent: gridComponent }
        }
    ]
}
