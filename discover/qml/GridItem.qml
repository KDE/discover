/*
 *   Copyright 2010 Marco Martin <notmart@gmail.com>
 *   Copyright 2012 Aleix Pol Gonzalez <aleixpol@kde.org>
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

MouseArea {
    id: listItem

    //this defines if the item will emit clicked and look pressed on mouse down
    property alias enabled: listItem.enabled
    property alias containsMouse: listItem.containsMouse

    property bool checked: false

    property bool changeBackgroundOnPress: !listItem.checked
    hoverEnabled: true

    SystemPalette {
        id: palette
    }
    Rectangle {
        anchors.fill: parent
        color: listItem.containsMouse ? palette.light : palette.midlight
    }
}
