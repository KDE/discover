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
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami

Item
{
    property alias text: theLabel.text
    property real progress: 1.
    readonly property real margin: 5
    implicitWidth: theLabel.implicitWidth + margin*2
    implicitHeight: theLabel.implicitHeight + margin*2
    width: implicitWidth
    height: implicitHeight

    Rectangle {
        color: Kirigami.Theme.disabledTextColor
        anchors.fill: parent
        radius: parent.margin
    }

    Rectangle {
        anchors {
            fill: parent
            rightMargin: (1-parent.progress) * parent.width
        }
        color: Kirigami.Theme.highlightColor
        radius: parent.margin
    }

    Label {
        id: theLabel
        anchors.centerIn: parent
        color: Kirigami.Theme.highlightedTextColor
    }
}
