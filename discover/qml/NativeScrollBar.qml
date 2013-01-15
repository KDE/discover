/*
 *   Copyright (C) 2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import org.kde.muon.discover 1.0

NativeScrollBar {
    id: scroll
    property QtObject flickableItem: null

    orientation: Qt.Vertical
    minimum: 0
    maximum: Math.max(flickableItem.contentHeight-flickableItem.height, 0)
    opacity: maximum!=minimum ? 1 : 0

    onValueChanged: flickableItem.contentY=value
    onFlickableItemChanged: flickableItem.boundsBehavior=Flickable.StopAtBounds

    Behavior on opacity { NumberAnimation { easing.type: Easing.OutQuad; duration: 500 } }

    Connections {
        target: scroll.flickableItem
        onContentYChanged: {
            if(flickableItem.movingVertically) {
                scroll.value=flickableItem.contentY
            }
        }
    }
}
