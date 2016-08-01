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
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

ColumnLayout {
    id: topView
    property alias sortRole: appsModel.stringSortRole
    property alias filteredCategory: appsModel.filteredCategory
    property Component roleDelegate: null
    property string title: ""
    property bool extended: false
    readonly property alias titleHeight: title.height

    spacing: Kirigami.Units.gridUnit
    Label {
        id: title
        text: topView.title
        Layout.fillWidth: true
        font.weight: Font.Bold
        Layout.minimumHeight: paintedHeight*1.5
    }
    Repeater {
        id: rep
        model: PaginateModel {
            pageSize: 5
            staticRowCount: true
            sourceModel: ApplicationProxyModel {
                id: appsModel
                sortOrder: Qt.DescendingOrder
//                 onRowsInserted: sortModel()
            }
        }
        delegate: ConditionalLoader {
            Layout.fillWidth: parent
            condition: model["name"] !== undefined
            componentFalse: Item {}
            componentTrue: ApplicationDelegate {
                application: model.application
            }
        }
    }
}
