/***************************************************************************
 *   Copyright © 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.GlobalDrawer {
    id: drawer
    anchors.fill: parent
    title: i18n("Discover")
    titleIcon: "plasmadiscover"

    function itemsFilter(actions, items) {
        var ret = [];
        for(var v in actions)
            ret.push(actions[v]);

        return ret.concat(items);
    }

    TextField {
        id: searchWidget
        anchors {
            left: parent.left;
            right: parent.right;
        }
        enabled: window.stack.currentItem!=null && window.stack.currentItem.searchFor!=null
        focus: true

        placeholderText: (!searchWidget.enabled || window.stack.currentItem.title == "") ? i18n("Search...") : i18n("Search in '%1'...", window.stack.currentItem.title)
        onTextChanged: searchTimer.running = true
        onEditingFinished: if(text == "" && backAction.enabled) {
            backAction.action.trigger()
        }

        Timer {
            id: searchTimer
            running: false
            repeat: false
            interval: 200
            onTriggered: {
                var ret = window.stack.currentItem.searchFor(searchWidget.text)
                if (ret === true)
                    backAction.action.trigger()
            }
        }
    }

    Kirigami.Action {
        id: configureMenu
        text: i18n("Configure...")
        iconName: "settings-configure"

        TopLevelPageData {
            id: sources
            text: i18n("Configure Sources...")
            iconName: "repository"
            shortcut: "Alt+S"
            component: topSourcesComp
        }
        ActionBridge {
            id: bindings
            action: app.action("options_configure_keybinding");
        }

        Instantiator {
            id: advanced
            model: MessageActionsModel {}
            delegate: ActionBridge { action: model.action }

            property var objects: []
            onObjectAdded: objects.push(object)
            onObjectRemoved: objects = objects.splice(configureMenu.children.indexOf(object))
        }

        children: [sources, bindings].concat(advanced.objects)
    }

    Kirigami.Action {
        id: helpMenu
        text: i18n("Help...")
        iconName: "system-help"

        ActionBridge { action: app.action("help_about_app"); }
        ActionBridge { action: app.action("help_report_bug"); }
    }

    actions: itemsFilter(window.awesome, [ configureMenu, helpMenu ])
    modal: Helpers.isCompact
    handleVisible: Helpers.isCompact

    states: [
        State {
            name: "full"
            when: !Helpers.isCompact
            PropertyChanges { target: drawer; opened: true }
        }
    ]
}
