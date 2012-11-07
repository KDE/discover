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

Item {
    id: parentItem
    property Component header: null
    property alias delegate: gridRepeater.delegate
    property alias model: gridRepeater.model
    
    property real actualWidth: width
    property real cellWidth: Math.min(200, actualWidth)
    property real cellHeight: cellWidth/1.618 //tau
    
    Flickable {
        id: viewFlickable
        anchors.fill: parent
        contentHeight: view.height+headerLoader.height
        
        Loader {
            id: headerLoader
            sourceComponent: parentItem.header
            anchors {
                left: view.left
                right: view.right
                top: parent.top
            }
        }
        
        Flow
        {
            id: view
            width: Math.floor(actualWidth/cellWidth)*(cellWidth+spacing)
            spacing: 5
            anchors {
                top: headerLoader.bottom
                horizontalCenter: parent.horizontalCenter
            }
            
            Repeater {
                id: gridRepeater
            }
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: viewFlickable
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
    }
}
