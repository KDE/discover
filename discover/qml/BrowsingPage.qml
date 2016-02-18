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

ScrollView {
    id: view
    readonly property string title: ""
    readonly property string icon: "go-home"
    flickableItem.flickableDirection: Flickable.VerticalFlick
    clip: true

    function searchFor(text) {
        Navigation.openApplicationList("edit-find", i18n("Search..."), null, text)
    }

    ColumnLayout
    {
        readonly property real margin: SystemFonts.generalFont.pointSize/2
        width: view.viewport.width-margin*2
        x: margin

        FeaturedBanner {
            Layout.fillWidth: true
            Layout.preferredHeight: 310
        }

        CategoryDisplay {
            Layout.fillWidth: true
        }
        Item {
            height: parent.margin
        }
    }
}
