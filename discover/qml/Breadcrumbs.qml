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
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import org.kde.kquickcontrolsaddons 2.0

RowLayout {
    id: bread
    readonly property int count: pageStack.depth
    property StackView pageStack: null
    spacing: 0
    anchors {
        top: parent.top
        bottom: parent.bottom
    }
    Repeater
    {
        model: bread.pageStack.depth
        delegate: RowLayout {
            spacing: 0
            QIconItem {
                visible: index > 0
                width: 16
                height: width
                icon: "arrow-right"
            }
            MuonToolButton {
                Layout.fillHeight: true

                property var currentPage: bread.pageStack.get(modelData, false)

                iconName: currentPage.icon
                onClicked: bread.doClick(index)
                text: currentPage.title
                enabled: bread.pageStack.depth!=(modelData+1)
                checkable: checked
            }
        }
    }
    function doClick(index) {
        var pos = bread.pageStack.depth
        for(; pos>(index+1); --pos) {
            bread.pageStack.pop(pos>index)
        }
    }
}
