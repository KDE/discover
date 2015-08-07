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
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1

ToolBar
{
    id: root
    Layout.preferredHeight: backAction.height+2
    Layout.fillWidth: true
    property Item search: searchWidget

    ExclusiveGroup {
        id: appTabs
    }

    RowLayout {
        id: layout
        spacing: 1
        anchors.fill: parent

        ToolButton {
            id: backAction
            objectName: "back"
            action: Action {
                shortcut: "Alt+Up"
                iconName: "go-previous"
                enabled: window.navigationEnabled && breadcrumbsItem.count>1
                tooltip: i18n("Back")
                onTriggered: { breadcrumbsItem.popItem(false) }
            }
        }

        ConditionalLoader {
            condition: app.isCompact
            componentFalse: RowLayout {
                Repeater {
                    model: window.awesome
                    delegate: Button {
                        enabled: modelData.enabled
                        checkable: modelData.checkable
                        checked: modelData.checked
                        onClicked: modelData.trigger();
                        iconName: modelData.iconName
                        text: modelData.text
                        tooltip: modelData.shortcut
                        exclusiveGroup: appTabs
                    }
                }
            }
            componentTrue: Button {
                text: appTabs.current ? appTabs.current.text: ""
                menu: Menu {
                    id: menu
                    Instantiator {
                        model: window.awesome
                        MenuItem {
                            enabled: modelData.enabled
                            checkable: modelData.checkable
                            checked: modelData.checked
                            onTriggered: modelData.trigger();
                            iconName: modelData.iconName
                            text: modelData.text
//                             tooltip: modelData.shortcut
                            exclusiveGroup: appTabs
                        }
                        onObjectAdded: menu.insertItem(index, object)
                        onObjectRemoved: menu.removeItem(object)
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: !app.isCompact
        }
        TextField {
            id: searchWidget
            Layout.fillWidth: app.isCompact
            placeholderText: i18n("Search...")
            focus: true
            enabled: pageStack.currentItem!=null && pageStack.currentItem.searchFor!=null

            onTextChanged: searchTimer.running = true
            onEditingFinished: if(searchWidget.text == "" && backAction.enabled) {
                backAction.trigger()
            }
        }
        ToolButton {
            id: button
            iconName: "preferences-other"
            tooltip: i18n("Configure and learn about Muon Discover")
            onClicked: {
                var pos = mapToItem(window, 0, height);
                app.showMenu(pos.x, pos.y);
            }
        }
    }
}
