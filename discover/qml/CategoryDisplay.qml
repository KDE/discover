/*
 *   Copyright (C) 2012-2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

RowLayout
{
    id: page
    property alias category: categoryModel.displayedCategory
    readonly property bool extended: !app.isCompact && grid.count>5

    spacing: 7

    ApplicationsTop {
        id: top
        Layout.fillHeight: true
        Layout.fillWidth: true
        sortRole: "sortableRating"
        filteredCategory: page.category
        title: i18n("Popularity")
        extended: page.extended
        roleDelegate: Item {
            width: bg.width
            property variant model
            LabelBackground {
                id: bg
                anchors.centerIn: parent
                text: model.sortableRating.toFixed(2)
            }
        }
    }
    ApplicationsTop {
        id: top2
        Layout.fillHeight: true
        Layout.fillWidth: true
        visible: !app.isCompact
        sortRole: "ratingPoints"
        filteredCategory: page.category
        title: i18n("Rating")
        extended: page.extended
        roleDelegate: Rating {
            property variant model
            rating: model.rating
            height: 12
        }
    }

    GridItem {
        Layout.fillWidth: true
        clip: true
        anchors {
            top: parent.top
            topMargin: top.titleHeight-2
            bottom: top.bottom
            bottomMargin: -1
        }
        Layout.preferredWidth: page.width/2
        hoverEnabled: false
        GridView {
            id: grid
            anchors {
                fill: parent
                margins: 5
            }
            cellWidth: app.isCompact ? width : width/Math.floor(width/100)
            cellHeight: app.isCompact ? 30 : 60
            boundsBehavior: Flickable.StopAtBounds

            model: CategoryModel {
                id: categoryModel
                displayedCategory: null
            }

            delegate: CategoryDelegate {
                horizontal: app.isCompact
                width: grid.cellWidth
                height: grid.cellHeight
            }
        }
    }
}
