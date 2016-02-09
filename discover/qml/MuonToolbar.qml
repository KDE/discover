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
    property Item search: searchWidget
    Layout.preferredHeight: layout.Layout.preferredHeight

    Timer {
        id: searchTimer
        running: false
        repeat: false
        interval: 200
        onTriggered: { window.stack.currentItem.searchFor(root.search.text) }
    }

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: 0

        RowLayout {
            spacing: 1
            anchors.fill: parent
            Layout.alignment: Qt.AlignVCenter

            ToolButton {
                id: backAction
                objectName: "back"
                visible: !app.isCompact
                Layout.alignment: Qt.AlignVCenter
                action: Action {
                    shortcut: "Alt+Up"
                    iconName: "go-previous"
                    enabled: window.navigationEnabled && window.stack.depth>1
                    tooltip: i18n("Back")
                    onTriggered: { window.stack.pop() }
                }
            }

            Repeater {
                model: window.awesome
                delegate: MuonToolButton {
                    action: modelData
                }
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            TextField {
                id: searchWidget
                enabled: window.stack.currentItem!=null && window.stack.currentItem.searchFor!=null
                focus: true

                placeholderText: i18n("Search...")
                onTextChanged: searchTimer.running = true
                onEditingFinished: if(text == "" && backAction.enabled) {
                    backAction.action.trigger()
                }
            }

            ToolButton {
                id: button
                iconName: "application-menu"
                menu: moreMenu
            }
        }
    }
}
