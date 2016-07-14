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
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover.app 1.0
import QtQuick.Window 2.1
import org.kde.kcoreaddons 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

Kirigami.AbstractListItem
{
    id: delegateArea

    onClicked: {
        if (ListView.view)
            ListView.view.currentIndex = index
        Navigation.openApplication(application)
    }

    RowLayout {
        id: lowLayout
        anchors {
            leftMargin: 2
            left: parent.left
            right: parent.right
        }

        QIconItem {
            id: resourceIcon
            icon: model.icon

            Layout.alignment: Qt.AlignTop | Qt.AlignHCenter
            readonly property real contHeight: Math.max(conts.height * 0.8, 128)
            Layout.minimumWidth: contHeight
            Layout.minimumHeight: contHeight
            anchors.verticalCenter: parent.verticalCenter
        }

        ColumnLayout {
            id: conts
            Layout.fillWidth: true
            Layout.fillHeight: true

            Item { height: 3; width: 3 }

            RowLayout {
                Layout.fillWidth: true
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: name
                    font: SystemFonts.titleFont
                }
                Label {
                    text: category[0]
                }
            }

            Rectangle {
                color: Kirigami.Theme.linkColor
                Layout.fillWidth: true
                height: 1
            }

            Label {
                Layout.fillWidth: true

                elide: Text.ElideRight
                text: comment
                maximumLineCount: 1
                clip: true
            }

            Label {
                Layout.fillWidth: true
                clip: true

                wrapMode: Text.WordWrap
                text: longDescription
                maximumLineCount: 5
            }

            InstallApplicationButton {
                id: installButton
                Layout.alignment: Qt.AlignRight
                application: model.application
                canUpgrade: false
            }

            Item { height: 3; width: 3 }
        }

        ApplicationIndicator {
            id: indicator
            width: 5
            height: parent.height
            anchors.right: parent.right
        }
    }
}
