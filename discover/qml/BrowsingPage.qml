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

DiscoverPage
{
    title: i18n("Home")
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    function searchFor(text) {
        if (text.length === 0)
            return;
        Navigation.openCategory(null, "")
    }
    pageHeader: Item {}
    CategoryDisplay {
        category: null
        width: parent.width
        background: "https://c2.staticflickr.com/8/7193/6900377481_76367f973a_o.jpg"

        ApplicationsTop {
            Layout.fillWidth: true
            Layout.leftMargin: Kirigami.Units.gridUnit
            Layout.rightMargin: Kirigami.Units.gridUnit
            sortRole: "ratingCount"
            title: i18n("Most Popular")
            extended: true
        }
    }
}
