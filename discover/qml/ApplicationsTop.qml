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

ColumnLayout {
    id: topView
    property alias sortRole: appsModel.stringSortRole
    property alias filteredCategory: appsModel.filteredCategory
    property Component roleDelegate: null
    property string title: ""
    property bool extended: false
    readonly property alias titleHeight: title.height

    Label {
        id: title
        text: topView.title
        Layout.fillWidth: true
        font.weight: Font.Bold
        Layout.minimumHeight: paintedHeight*1.5
    }
    spacing: -1 //GridItem.border.width
    Repeater {
        id: rep
        model: PaginateModel {
            pageSize: 5
            sourceModel: ApplicationProxyModel {
                id: appsModel
                sortOrder: Qt.DescendingOrder
//                 onRowsInserted: sortModel()
            }
        }
        delegate: GridItem {
                    Layout.fillWidth: true
                    Layout.minimumHeight: title.paintedHeight*(topView.extended ? 3.5 : 2.5)

                    RowLayout {
                        id: layo
                        anchors {
                            fill: parent
                            margins: 2
                        }
                        QIconItem {
                            Layout.fillHeight: true
                            Layout.minimumWidth: height
                            icon: model.icon
                        }
                        ColumnLayout {
                            Layout.fillHeight: true
                            Layout.fillWidth: true

                            Label {
                                id: nameItem
                                Layout.fillWidth: true
                                text: name
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                            }
                            Label {
                                Layout.preferredWidth: nameItem.Layout.preferredWidth
                                visible: topView.extended
                                text: category[0]
                                elide: Text.ElideRight
                                verticalAlignment: Text.AlignVCenter
                                opacity: 0.6
                            }
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
