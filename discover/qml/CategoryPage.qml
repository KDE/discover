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
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Page {
    id: page
    property QtObject category
    property real actualWidth: width-Math.pow(width/70, 2)
    
    function searchFor(text) {
        if(category)
            Navigation.openApplicationList(category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList("edit-find", i18n("Search..."), category, text)
    }
    
    Component {
        id: categoryDelegate
        ListItem {
            property int minCellWidth: 130
            width: parent.width/Math.ceil(parent.width/minCellWidth)-10
            height: 100
            enabled: true
            Column {
                anchors.centerIn: parent
                width: parent.width
                spacing: 10
                QIconItem {
                    icon: decoration
                    width: 40; height: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label {
                    text: display
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
            onClicked: {
                switch(categoryType) {
                    case CategoryModel.CategoryType:
                        Navigation.openApplicationList(category.icon, category.name, category, "")
                        break;
                    case CategoryModel.SubCatType:
                        Navigation.openCategory(category)
                        break;
                }
            }
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: flick
        anchors {
            top: parent.top
            right: parent.right 
            bottom: parent.bottom
        }
    }

    Flickable {
        id: flick
        anchors {
            fill: parent
            bottomMargin: 10
        }
        contentHeight: conts.height
        
        Column {
            id: conts
            width: page.actualWidth
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 10
            Loader {
                width: parent.width
                Component {
                    id: categoryHeader
                    CategoryHeader {
                        category: page.category
                        height: 128
                    }
                }
                
                Component {
                    id: featured
                    FeaturedBanner {
                        height: 310
                        clip: true
                    }
                }
                sourceComponent: category==null ? featured : categoryHeader
            }
            
            Flow {
                width: parent.width
                spacing: 10
                Repeater {
                    model: CategoryModel {
                        displayedCategory: page.category
                    }
                    delegate: categoryDelegate
                }
            }
            
            Item {
                height: Math.min(200, page.height/2)
                width: parent.width
                ApplicationsTop {
                    width: parent.width/2-5
                    anchors {
                        top: parent.top
                        left: parent.left
                        bottom: parent.bottom
                    }
                    sortRole: "popcon"
                    filteredCategory: page.category
                    title: i18n("Popularity Contest")
                    roleDelegate: Label { property variant model; text: i18n("points: %1", model.popcon) }
                }
                ApplicationsTop {
                    width: parent.width/2-5
                    anchors {
                        top: parent.top
                        right: parent.right
                        bottom: parent.bottom
                    }
                    sortRole: "ratingPoints"
                    filteredCategory: page.category
                    title: i18n("Best Ratings")
                    roleDelegate: Rating { property variant model; rating: model.rating; height: 10 }
                }
            }
        }
    }
}
