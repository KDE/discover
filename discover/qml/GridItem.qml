/*
 *   Copyright 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Library General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

///This is a fork of ListItem so that it can be used for GridView

import QtQuick 2.1
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0

MouseArea {
    id: listItem

    default property alias content: paddingItem.data

    property int internalMargin: 5
    readonly property color highlightColor: Qt.lighter(DiscoverSystemPalette.highlight)
    readonly property real internalWidth: width - 2*internalMargin
    readonly property real internalHeight: height - 2*internalMargin

    hoverEnabled: !Helpers.isCompact

    BasicListItem {
        anchors.fill: parent
        color: listItem.containsMouse || listItem.pressed ? listItem.highlightColor : DiscoverSystemPalette.button
        border.color: DiscoverSystemPalette.mid
        border.width: 1
    }

    Item {
        id: paddingItem
        anchors {
            fill: parent
            leftMargin: listItem.internalMargin
            topMargin: listItem.internalMargin
            rightMargin: listItem.internalMargin
            bottomMargin: listItem.internalMargin
        }
    }

}
