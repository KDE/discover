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
    Layout.preferredHeight: layout.Layout.preferredHeight.height+2
    Layout.fillWidth: true
    property Item search: app.isCompact ? compactSearch : null

    ExclusiveGroup {
        id: appTabs
    }
    Timer {
        id: searchTimer
        running: false
        repeat: false
        interval: 200
        onTriggered: { stackView.currentItem.searchFor(root.search.text) }
    }

    ColumnLayout {
        RowLayout {
            id: layout
            spacing: 1
            anchors.fill: parent

            ToolButton {
                id: backAction
                objectName: "back"
                visible: !app.isCompact
                action: Action {
                    shortcut: "Alt+Up"
                    iconName: "go-previous"
                    enabled: window.navigationEnabled && stackView.depth>1
                    tooltip: i18n("Back")
                    onTriggered: { stackView.pop() }
                }
            }

            Repeater {
                model: window.awesome
                delegate: Button {
                    enabled: modelData.enabled
                    checkable: modelData.checkable
                    checked: modelData.checked
                    onClicked: modelData.trigger();
                    iconName: modelData.iconName
                    text: app.isCompact ? "" : modelData.text
                    tooltip: i18n("%1 (%2)", modelData.text, modelData.shortcut)
                    exclusiveGroup: appTabs
                }
            }

            Item {
                Layout.fillWidth: true
            }
            ConditionalLoader {
                condition: app.isCompact
                enabled: stackView.currentItem!=null && stackView.currentItem.searchFor!=null

                componentTrue: Button {
                    iconName: "search"
                    checkable: true
                    onCheckedChanged: {
                        compactSearch.visible = checked
                        compactSearch.focus = true
                    }
                }
                componentFalse: TextField {
                    id: searchWidget
                    Component.onCompleted: {
                        root.search = searchWidget
                    }
                    focus: true

                    placeholderText: i18n("Search...")
                    onTextChanged: searchTimer.running = true
                    onEditingFinished: if(text == "" && backAction.enabled) {
                        backAction.trigger()
                    }
                }

            }
            ToolButton {
                id: button
                iconName: "application-menu"
                tooltip: i18n("Configure and learn about Muon Discover")
                onClicked: {
                    var pos = mapToItem(window, 0, height);
                    app.showMenu(pos.x, pos.y);
                }
            }
        }
        TextField {
            id: compactSearch
            visible: false
            Layout.fillWidth: true

            placeholderText: i18n("Search...")
            onTextChanged: searchTimer.running = true
            onEditingFinished: if(text == "" && backAction.enabled) {
                backAction.trigger()
            }
        }
    }
}
