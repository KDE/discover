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
    property var category
    property string search

    spacing: 0

    signal setCategory(QtObject category)

    Kirigami.Action {
        id: searchAction
        text: bread.search? i18n("Search: %1", bread.search) : ""
        onTriggered: bread.setCategory(category);
    }

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            property QtObject category
            text: category.name
            onTriggered: bread.setCategory(category)
        }
    }

    function breadcrumbs(search, category) {
        var ret = [];

        while(category) {
            var categoryAction = categoryActionComponent.createObject(rep, { category: category })
            ret.unshift(categoryAction)
            category = category.parent
        }
        if (search !== "")
            ret.unshift(searchAction);
        return ret
    }

    Repeater {
        id: rep
        model: breadcrumbs(bread.search, bread.category)

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

                action: modelData
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
