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

ListView {
    id: view
    property alias sortRole: appsModel.stringSortRole
    property alias filteredCategory: appsModel.filteredCategory
    property Component roleDelegate: null
    property string title: ""

    interactive: false
    model: ApplicationProxyModel {
        id: appsModel
        sortOrder: Qt.DescendingOrder
        onRowsInserted: sortModel()
    }
    header: Label {
        text: ListView.view.title
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        font.weight: Font.Bold
        height: paintedHeight*1.5
    }
    delegate: ListItem {
                width: ListView.view.width
                height: (view.height-nameLabel.paintedHeight*1.5-view.spacing*5)/5
                enabled: true
                QIconItem {
                    id: iconItem
                    anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                    height: parent.height*0.9
                    width: height
                    icon: model.icon
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
                    sourceComponent: view.roleDelegate
                    onItemChanged: item.model=model
                }
                onClicked: Navigation.openApplication(application)
            }
}
