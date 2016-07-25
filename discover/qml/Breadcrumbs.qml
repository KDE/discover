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
import "navigation.js" as Navigation

RowLayout {
    id: bread
    property var category
    property string search

    spacing: 2

    Kirigami.Action {
        id: searchAction
        text: bread.search? i18n("Search: %1", bread.search) : ""
        onTriggered: Navigation.openCategory(category, "");
    }

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            property QtObject category
            text: category.name
            onTriggered: Navigation.openCategory(category, bread.search)
        }
    }

    function breadcrumbs(search, category) {
        var ret = [];

        if (category) //skip the first one
            category = category.parent;

        while(category) {
            //TODO: check for leaks
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
            Text {
                visible: index > 0
                text: ">"
                color: button.textColor
            }
            LinkButton {
                id: button
                Layout.fillHeight: true

                text: modelData.text
                onClicked: modelData.trigger()
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
