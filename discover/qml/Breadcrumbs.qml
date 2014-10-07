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

Item {
    id: bread
    property alias count: items.count
    property Item pageStack: null
    signal poppedPages
    
    function currentItem() {
        return items.count=="" ? null : items.get(items.count-1).display
    }
    
    function pushItem(icon, text) {
        items.append({"decoration": icon, "display": text})
    }
    
    function popItem(last) {
        items.remove(items.count-1)
        var page = pageStack.pop(undefined, last)
        page.destroy(1000)
        
        if(last)
            poppedPages()
    }
    
    function doClick(index) {
        var pos = items.count-index-1
        for(; pos>0; --pos) {
            bread.popItem(pos>1)
        }
    }
    
    function removeAllItems() {
        var pos = items.count-1
        for(; pos>0; --pos) {
            items.remove(items.count-1)
        }
    }

    Row {
        spacing: 0
        Repeater
        {
            id: view

            model: ListModel { id: items }
            delegate: Button {
                height: bread.height
                iconName: decoration
                onClicked: doClick(index)
                text: display
                enabled: items.count-index>1
                checkable: checked
            }

//TODO: make sure the right-most button is visible
//         onCountChanged: view.positionViewAtEnd()
        }
    }
}
