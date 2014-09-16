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
import org.kde.plasma.core 2.0
import org.kde.muon 1.0
import org.kde.muon.discover 1.0
import "navigation.js" as Navigation

Column {
    id: topView
    property alias sortRole: appsModel.stringSortRole
    property alias filteredCategory: appsModel.filteredCategory
    property Component roleDelegate: null
    property string title: ""

    height: 200
    Label {
        text: topView.title
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        font.weight: Font.Bold
        height: paintedHeight*1.5
    }
    Repeater {
        model: PaginateModel {
            pageSize: 5
            sourceModel: ApplicationProxyModel {
                id: appsModel
                sortOrder: Qt.DescendingOrder
//                 onRowsInserted: sortModel()
            }
        }
        delegate: MouseArea {
                    width: topView.width
                    height: (topView.height-nameLabel.paintedHeight*1.3-topView.spacing*5)/5


                    IconItem {
                        id: iconItem
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        height: parent.height*0.5
                        width: height
                        source: model.icon
                    }
                    Label {
                        id: nameLabel
                        anchors {
                            left: iconItem.right
                            right: pointsLabel.left
                            verticalCenter: parent.verticalCenter
                            leftMargin: 5
                        }
                        text: name
                        elide: Text.ElideRight
                    }
                    Loader {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        id: pointsLabel
                        sourceComponent: topView.roleDelegate
                        onItemChanged: item.model=model
                    }
                    onClicked: Navigation.openApplication(application)
                }
    }
}
