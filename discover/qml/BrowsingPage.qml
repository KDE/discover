/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import org.kde.discover 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 1.0 as Kirigami

Kirigami.Page
{
    title: i18n("Discover")
    readonly property string icon: "go-home"
    ScrollView {
        id: view
        anchors.fill: parent
        flickableItem.flickableDirection: Flickable.VerticalFlick

        function searchFor(text) {
            if (text === "")
                return;
            Navigation.openApplicationList(null, text)
        }

        ColumnLayout
        {
            readonly property real margin: SystemFonts.generalFont.pointSize
            width: view.viewport.width-margin*2
            x: margin

            FeaturedBanner {
                Layout.fillWidth: true
                Layout.preferredHeight: parent.margin*30
            }

            CategoryDisplay {
                spacing: parent.margin
                Layout.fillWidth: true
            }
            Item {
                height: parent.margin
            }
        }
    }
}
