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
    property alias application: installButton.application

    onClicked: {
        if (ListView.view)
            ListView.view.currentIndex = index
        Navigation.openApplication(application)
    }

    RowLayout {
        id: lowLayout
        anchors {
            left: parent.left
            right: parent.right
            leftMargin: Kirigami.Units.largeSpacing
            topMargin: Kirigami.Units.largeSpacing
            bottomMargin: Kirigami.Units.largeSpacing
        }
        spacing: Kirigami.Units.largeSpacing

        QIconItem {
            id: resourceIcon
            icon: application.icon

            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
            readonly property real contHeight: Math.max(delegateArea.height * 0.8, 128)
            width: contHeight
            height: contHeight
            anchors.verticalCenter: parent.verticalCenter
        }

        ColumnLayout {
            id: conts
            Layout.fillWidth: true
            Layout.fillHeight: true

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
                    color: Kirigami.Theme.linkColor
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
                font: SystemFonts.titleFont
                clip: true
            }

            Item { height: Kirigami.Units.largeSpacing; width: 3 }

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
                canUpgrade: false
            }
        }

        ApplicationIndicator {
            id: indicator
            width: 5
            height: parent.height
            anchors.right: parent.right
        }
    }
}
