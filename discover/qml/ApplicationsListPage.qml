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

import QtQuick 1.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation
import org.kde.muon.discover 1.0
import org.kde.muon 1.0

Page {
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
        appsModel.stringSortRole = role
        appsModel.sortOrder=sorting
        page.sectionProperty = section
        page.sectionDelegate = role=="canUpgrade" ? installedSectionDelegate : defaultSectionDelegate
    }
    
    tools: Row {
            id: buttonsRow
            width: 100
            height: theme.defaultFont.pointSize*2
            visible: page.visible
            spacing: 3

            MuonToolButton {
                id: sortButton
                icon: "view-sort-ascending"
                onClicked: menu.open()

                ActionGroup { id: sortActionGroup }
                Menu {
                    id: menu
                    MenuItem {
                        id: nameItem
                        text: i18n("Name")
                        onClicked: page.changeSorting("name", Qt.AscendingOrder, "")
                        checked: appsModel.stringSortRole=="name"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(nameItem)
                    }
                    MenuItem {
                        id: ratingItem
                        text: i18n("Rating")
                        onClicked: page.changeSorting("sortableRating", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="sortableRating"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(ratingItem)
                    }
                    MenuItem {
                        id: buzzItem
                        text: i18n("Buzz")
                        onClicked: page.changeSorting("ratingPoints", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="ratingPoints"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(buzzItem)
                    }
                    MenuItem {
                        id: popularityItem
                        text: i18n("Popularity")
                        onClicked: page.changeSorting("popcon", Qt.DescendingOrder, "")
                        checked: appsModel.stringSortRole=="popcon"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(popularityItem)
                    }
                    MenuItem {
                        id: originItem
                        text: i18n("Origin")
                        onClicked: page.changeSorting("origin", Qt.DescendingOrder, "origin")
                        checked: appsModel.stringSortRole=="origin"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(originItem)
                    }
                    MenuItem {
                        id: installedItem
                        text: i18n("Installed")
                        onClicked: page.changeSorting("canUpgrade", Qt.DescendingOrder, "canUpgrade")
                        checked: appsModel.stringSortRole=="canUpgrade"
                        checkable: true
                        Component.onCompleted: sortActionGroup.addAction(installedItem)
                    }
                }
            }

            ActionGroup { id: shownActionGroup }
            MuonToolButton {
                id: listViewShown
                icon: "tools-wizard"
                onClicked: shownMenu.open()
                Menu {
                    id: shownMenu
                    MenuItem {
                        id: itemList
                        property string type: "list"
                        text: i18n("List")
                        checkable: true
                        checked: page.state==type
                        onClicked: page.state=type
                        Component.onCompleted: shownActionGroup.addAction(itemList)
                    }
                    MenuItem {
                        id: itemGrid
                        property string type: "grid2"
                        text: i18n("Grid")
                        checkable: true
                        checked: page.state==type
                        onClicked: page.state=type
                        Component.onCompleted: shownActionGroup.addAction(itemGrid)
                    }
                    MenuItem { separator: true }
                    MenuItem {
                        checkable: true
                        checked: appsModel.shouldShowTechnical
                        onClicked: {
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
                rightMargin: apps.proposedMargin
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
