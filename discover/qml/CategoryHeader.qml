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
import org.kde.qtextracomponents 0.1
import org.kde.plasma.components 0.1

Item {
    property QtObject category: null
    
    QIconItem {
        id: iconItem
        icon: category.icon
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            margins: 15
        }
        width: height
    }
    
    Label {
        anchors {
            verticalCenter: parent.verticalCenter
            left: iconItem.right
            leftMargin: 50
        }
        font.pixelSize: parent.height*0.5
        text: category.name
    }
}
