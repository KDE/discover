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
import org.kde.muon.discover 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

Column {
    id: topView
    property alias sortRole: appsModel.stringSortRole
    property alias filteredCategory: appsModel.filteredCategory
    property Component roleDelegate: null
    property string title: ""

    Layout.preferredHeight: childrenRect.height
    Layout.preferredWidth: 250
    Label {
        id: title
        text: topView.title
        width: parent.width
        horizontalAlignment: Text.AlignHCenter
        font.weight: Font.Bold
        height: paintedHeight*1.5
    }
    spacing: 5
    Repeater {
        model: PaginateModel {
            pageSize: 5
            sourceModel: ApplicationProxyModel {
                id: appsModel
                sortOrder: Qt.DescendingOrder
//                 onRowsInserted: sortModel()
            }
        }
        delegate: GridItem {
                    width: topView.width
                    height: title.paintedHeight*2.5

                    RowLayout {
                        anchors.fill: parent
                        QIconItem {
                            Layout.fillHeight: true
                            Layout.minimumWidth: height
                            icon: model.icon
                        }
                        Label {
                            Layout.fillHeight: true
                            text: name
                            elide: Text.ElideRight
                        }
                        Item {
                            Layout.fillWidth: true
                        }
                        Loader {
                            Layout.fillHeight: true
                            Layout.minimumWidth: item.width
                            sourceComponent: topView.roleDelegate
                            onItemChanged: item.model=model
                        }
                    }
                    onClicked: Navigation.openApplication(application)
                }
    }
}
