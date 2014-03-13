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
import org.kde.plasma.core 2.0 as PlasmaCore

MouseArea {
    id: listItem
    default property alias content: paddingItem.data

    //this defines if the item will emit clicked and look pressed on mouse down
    property alias enabled: listItem.enabled
    property alias containsMouse: listItem.containsMouse

    property bool checked: false

    width: parent ? parent.width : childrenRect.width
    height: paddingItem.childrenRect.height + background.margins.top + background.margins.bottom

    property bool changeBackgroundOnPress: !listItem.checked
    hoverEnabled: true

    PlasmaCore.FrameSvgItem {
        id : background
        imagePath: "widgets/listitem"
        prefix: (listItem.checked || listItem.pressed) ? "pressed" : "normal"

        anchors.fill: parent
        opacity: listItem.containsMouse && !listItem.pressed ? 0.5 : 1
        Behavior on opacity { NumberAnimation { duration: 200 } }
    }
    PlasmaCore.SvgItem {
        svg: PlasmaCore.Svg {imagePath: "widgets/listitem"}
        elementId: "separator"
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
        }
        height: naturalSize.height
        visible: listItem.sectionDelegate || (typeof(index) != "undefined" && index > 0 && !listItem.checked && !listItem.pressed)
    }

    Item {
        id: paddingItem
        anchors {
            fill: background
            leftMargin: background.margins.left
            topMargin: background.margins.top
            rightMargin: background.margins.right
            bottomMargin: background.margins.bottom
        }
    }
}
