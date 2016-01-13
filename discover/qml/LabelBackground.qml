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

Rectangle
{
    property alias text: theLabel.text
    property bool progressing: false
    property real progress: 1.
    color: pal.dark
    radius: 5
    width: theLabel.implicitWidth + 10
    height: theLabel.implicitHeight + 10

    SystemPalette {
        id: pal
    }

    Rectangle {
        color: pal.light
        opacity: 0.3
        visible: parent.progressing
        anchors {
            fill: parent
            leftMargin: parent.progress * parent.width
        }
    }

    Label {
        id: theLabel
        anchors.centerIn: parent
        color: pal.highlightedText
    }
}
