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
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Breadcrumbs {
    id: root
    property var category
    property string search

    spacing: 2

    Kirigami.Action {
        id: searchAction
        text: root.search ? i18n("Search: %1", root.search) : ""
        onTriggered: Navigation.openCategory(category, "");
    }

    Component {
        id: categoryActionComponent
        Kirigami.Action {
            property QtObject category
            text: category.name
            onTriggered: Navigation.openCategory(category, root.search)
            enabled: category != root.category
        }
    }

    function breadcrumbs(search, category) {
        var ret = [];

        while(category) {
            //TODO: check for leaks
            var categoryAction = categoryActionComponent.createObject(searchAction, { category: category })
            ret.unshift(categoryAction)
            category = category.parent
        }
        ret.unshift(homeAction)
        if (search !== "")
            ret.unshift(searchAction);
        return ret
    }
    model: breadcrumbs(search, category)
}
