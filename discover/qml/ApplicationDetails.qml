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
import QtQuick.Window 2.1
import org.kde.muon.discover 1.0 as Discover
import org.kde.muon 1.0

ColumnLayout
{
    property alias isInstalling: installButton.isActive
    property alias application: installButton.application
    spacing: 10

    InstallApplicationButton {
        id: installButton
        anchors.horizontalCenter: parent.horizontalCenter
        additionalItem:  Rating {
            property QtObject ratingInstance: application.rating
            visible: ratingInstance!=null
            rating:  ratingInstance==null ? 0 : ratingInstance.rating
        }
    }

    Grid {
        Layout.fillWidth: true
        columns: 2
        spacing: 0
        Label { text: i18n("Total Size: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
        Label { text: application.sizeDescription; width: parent.width/2; elide: Text.ElideRight }
        Label { text: i18n("Version: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
        Label { text: application.packageName+" "+(application.isInstalled ? application.installedVersion : application.availableVersion); width: parent.width/2; elide: Text.ElideRight }
        Label { text: i18n("Homepage: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
        Label {
            text: application.homepage
            MouseArea {
                anchors.fill: parent
                onClicked: Qt.openUrlExternally(application.homepage);
            }
            SystemPalette { id: palette }
            color: palette.highlight
            font.underline: true
            width: parent.width/2
            elide: Text.ElideRight
        }
        Label { text: i18n("License: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
        Label { text: application.license; width: parent.width/2; elide: Text.ElideRight }
    }
    Button {
        anchors.horizontalCenter: parent.horizontalCenter
        visible: application.isInstalled && application.canExecute
        text: i18n("Launch")
        onClicked: application.invokeApplication()
    }
}
