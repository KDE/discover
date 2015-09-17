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

GridItem {
    id: categoryItem
    property bool horizontal: false
    height: horizontal ? nameLabel.paintedHeight*2.5 : layout.height+10
    enabled: true

    GridLayout {
        id: layout
        rows: categoryItem.horizontal ? 1 : 2
        columns: categoryItem.horizontal ? 2 : 1

        anchors.centerIn: parent
        width: parent.width
        columnSpacing: 10
        rowSpacing: 5
        Item {
            Layout.fillWidth: !categoryItem.horizontal
            Layout.fillHeight: categoryItem.horizontal
            Layout.preferredWidth: categoryItem.horizontal ? nameLabel.paintedHeight*2 : 40
            Layout.preferredHeight: Layout.preferredWidth
            QIconItem {
                icon: decoration
                width: Math.min(parent.width, parent.height)
                height: width
                anchors.centerIn: parent
            }
        }
        Label {
            id: nameLabel
            text: display
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            wrapMode: Text.WordWrap
        }
    }
    onClicked: Navigation.openCategory(category)
}
