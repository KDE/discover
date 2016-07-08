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
import org.kde.kirigami 1.0 as Kirigami

RowLayout {
    id: bread
    readonly property QtObject pageStack: applicationWindow().pageStack
    property QtObject currentPage: null

    spacing: 0
    anchors {
        top: parent.top
        bottom: parent.bottom
    }
    Repeater
    {
        id: rep
        model: {
//             for(var i=0, c=bread.pageStack.depth; i<c; ++i) {
//                 var page = bread.pageStack.get(i);
//                 if (bread.currentPage == page) {
//                     return i;
//                 }
//             }
            return 0;
        }
        delegate: RowLayout {
            spacing: 0
            QIconItem {
                visible: index > 0
                width: button.Layout.preferredHeight/2
                height: width
                icon: "arrow-right"
            }
            ToolButton {
                id: button
                Layout.fillHeight: true

                readonly property QtObject currentPage: bread.pageStack.get(modelData, false)

                onClicked: bread.doClick(index)
                text: currentPage ? currentPage.title : "<null>"
                checkable: checked
            }
        }
    }
    function doClick(index) {
        var pos = bread.pageStack.depth
        for(; pos>(index+1); --pos) {
            bread.pageStack.pop(pos>index).destroy(2000);
        }
    }
}
