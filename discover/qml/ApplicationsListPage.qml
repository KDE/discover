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
import "navigation.js" as Navigation
import org.kde.muon.discover 1.0
import org.kde.muon 1.0

Item {
    id: page
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
    property real actualWidth: width-Math.pow(width/70, 2)
    property real proposedMargin: (width-actualWidth)/2
    property Component header: category==null ? null : categoryHeaderComponent
    property Component extendedToolBar: null
    clip: true
    
    onSearchChanged: appsModel.sortOrder = Qt.AscendingOrder
    
    function searchFor(text) {
        appsModel.search = text
    }
    
    ApplicationProxyModel {
        id: appsModel
        stringSortRole: "ratingPoints"
        sortOrder: Qt.DescendingOrder
        isShowingTechnical: category && category.shouldShowTechnical
        
        Component.onCompleted: sortModel()
    }

    function changeSorting(role, sorting, section) {
        console.log("changing to", role, "was", appsModel.stringSortRole);
        appsModel.stringSortRole = role
        appsModel.sortOrder=sorting
        page.sectionProperty = section
        page.sectionDelegate = role=="canUpgrade" ? installedSectionDelegate : defaultSectionDelegate
    }
    
    property Component tools: Row {
            height: SystemFonts.generalFont.pointSize*2
            visible: page.visible
            spacing: 3

            Loader {
                width: item ? item.width : 0
                sourceComponent: page.extendedToolBar
            }

            MuonToolButton {
                id: sortButton
                icon: "view-sort-ascending"
                onClicked: menu.popup()

                ExclusiveGroup { id: sortActionGroup }
                menu: Menu {
                    id: menu
                    MenuItem {
                        id: nameItem
                        text: i18n("Name")
                        onTriggered: page.changeSorting("name", Qt.AscendingOrder, "")
                        checked: appsModel.stringSortRole=="name"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                    MenuItem {
                        id: ratingItem
                        text: i18n("Rating")
                        onTriggered: page.changeSorting("sortableRating", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="sortableRating"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                    MenuItem {
                        id: buzzItem
                        text: i18n("Buzz")
                        onTriggered: page.changeSorting("ratingPoints", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="ratingPoints"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                    MenuItem {
                        id: popularityItem
                        text: i18n("Popularity")
                        onTriggered: page.changeSorting("popcon", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="popcon"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                    MenuItem {
                        id: originItem
                        text: i18n("Origin")
                        onTriggered: page.changeSorting("origin", Qt.DescendingOrder, "origin")
                        checked: appsModel.stringSortRole=="origin"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                    MenuItem {
                        id: installedItem
                        text: i18n("Installed")
                        onTriggered: page.changeSorting("canUpgrade", Qt.DescendingOrder, "canUpgrade")
                        checked: appsModel.stringSortRole=="canUpgrade"
                        checkable: true
                        exclusiveGroup: sortActionGroup
                    }
                }
            }

            MuonToolButton {
                id: listViewShown
                icon: "tools-wizard"
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
        CategoryHeader {
            id: categoryHeader
            category: page.category
            height: 100
            width: page.actualWidth
            proposedMargin: viewLoader.sourceComponent == listComponent ? page.proposedMargin : 0
        }
    }
    
    Loader {
        id: viewLoader
        anchors.fill: parent
    }
    
    Component {
        id: listComponent
        ApplicationsList {
            id: apps
            anchors.fill: parent
            section.property: page.sectionProperty
            section.delegate: page.sectionDelegate
            actualWidth: page.actualWidth
            
            header: page.header
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
            actualWidth: page.actualWidth
            minCellWidth: 200
            
            delegate: ApplicationsGridDelegate {
                height: width/1.618 //tau
                width: theGrid.cellWidth
            }
        }
    }
    
    state: preferList ? "list" : "grid2"
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
