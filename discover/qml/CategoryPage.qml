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
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

Item {
    id: page
    property QtObject category
    property real actualWidth: width-Math.pow(width/70, 2)
    property alias categories: categoryModel
    
    function searchFor(text) {
        if(text == "")
            return;
        if(category)
            Navigation.openApplicationList(category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList("edit-find", i18n("Search..."), category, text)
    }
    
    onVisibleChanged: {
        if(visible && !category)
            searchWidget.text = ""
    }
    
    Component {
        id: categoryDelegate
        GridItem {
            id: category
            property bool horizontal: flick.columnCount==1
            width: flick.cellWidth
            height: 100
            enabled: true

            GridLayout {
                rows: category.horizontal ? 1 : 2
                columns: category.horizontal ? 2 : 1

                anchors.centerIn: parent
                width: parent.width
                columnSpacing: 10
                rowSpacing: 10
                Item {
                    Layout.fillWidth: !category.horizontal
                    Layout.fillHeight: category.horizontal
                    Layout.preferredWidth: 40
                    Layout.preferredHeight: 40
                    QIconItem {
                        icon: decoration
                        anchors.centerIn: parent
                        width: 40
                        height: width
                    }
                }
                Label {
                    text: display
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }
            onClicked: Navigation.openCategory(category)
        }
    }
    
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
    ScrolledAwesomeGrid {
        id: flick
        anchors.fill: parent

        model: CategoryModel {
            id: categoryModel
            displayedCategory: page.category
        }
        actualWidth: page.actualWidth
        delegate: categoryDelegate
        header: category==null ? featured : categoryHeader
        footer: RowLayout {
                    height: top.Layout.preferredHeight+5
                    width: parent.width
                    ApplicationsTop {
                        id: top
                        Layout.fillWidth: true
                        sortRole: "sortableRating"
                        filteredCategory: page.category
                        title: i18n("Popularity")
                        roleDelegate: Label { property variant model; text: i18n("points: %1", model.sortableRating.toFixed(2)) }
                    }
                    ApplicationsTop {
                        id: top2
                        Layout.fillHeight: true
                        Layout.fillWidth: true
                        visible: parent.width > Layout.preferredWidth*2
                        sortRole: "ratingPoints"
                        filteredCategory: page.category
                        title: i18n("Rating")
                        roleDelegate: Rating {
                            property variant model
                            rating: model.rating
                            height: 12
                        }
                    }
                }
    }
}
