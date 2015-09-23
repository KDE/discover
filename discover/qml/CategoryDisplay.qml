/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

Item
{
    implicitHeight: categories.height

    property alias category: categoryModel.displayedCategory
    CategoryTop {
        id: categories
        category: parent.category
        anchors {
            top: parent.top
            right: parent.horizontalCenter
            bottom: parent.bottom
            left: parent.left
            rightMargin: 5
        }
    }

    GridItem {
        clip: true
        anchors {
            top: parent.top
            left: parent.horizontalCenter
            bottom: parent.bottom
            right: parent.right
            topMargin: categories.titleHeight-2
        }
        hoverEnabled: false
        GridView {
            id: grid
            anchors.fill: parent
            cellWidth: app.isCompact ? width : 100
            cellHeight: app.isCompact ? 30 : 60

            model: CategoryModel {
                id: categoryModel
                displayedCategory: null
            }

            delegate: CategoryDelegate {
                horizontal: app.isCompact
                width: grid.cellWidth
            }
        }
    }
}
