/*
 *   Copyright (C) 2012-2013 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import QtQuick.Window 2.2
import org.kde.discover.app 1.0

Flickable {
    readonly property int columnCount: Helpers.isCompact ? 1 : Math.max(Math.floor(conts.width/(minCellWidth+dataFlow.spacing*2)), 1)
    readonly property real cellWidth: (conts.width-(columnCount-1)*dataFlow.spacing)/columnCount
    readonly property alias count: dataRepeater.count
    readonly property int minCellWidth: SystemFonts.generalFont.pointSize*20
    property alias header: headerLoader.sourceComponent
    property alias footer: footerLoader.sourceComponent
    property alias delegate: dataRepeater.delegate
    property alias model: dataRepeater.model
    property alias section: sectionLoader.sourceComponent
    contentHeight: conts.height
    
    Column {
        id: conts
        width: parent.width-20
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 10
        Loader {
            id: headerLoader
            width: parent.width
        }
        Loader {
            id: sectionLoader
            width: parent.width
        }
        
        Flow {
            id: dataFlow
            width: parent.width
            spacing: 10
            Repeater {
                id: dataRepeater
            }
        }
        
        Loader {
            id: footerLoader
            width: parent.width
        }
    }
}
