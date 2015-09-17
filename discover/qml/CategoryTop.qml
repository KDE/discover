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

RowLayout {
    id: page
    property QtObject category: null
    implicitHeight: top.Layout.preferredHeight+5
    height: top.Layout.preferredHeight+5

    ApplicationsTop {
        id: top
        Layout.fillHeight: true
        Layout.fillWidth: true
        sortRole: "sortableRating"
        filteredCategory: page.category
        title: i18n("Popularity")
        roleDelegate: Label {
            property variant model
            text: i18n("points: %1", model.sortableRating.toFixed(2))
            verticalAlignment: Text.AlignVCenter
        }
    }
    ApplicationsTop {
        id: top2
        Layout.fillHeight: true
        Layout.fillWidth: true
        visible: !app.isCompact
        sortRole: "ratingPoints"
        filteredCategory: page.category
        title: i18n("Rating")
        roleDelegate: Rating {
            property variant model
            rating: model.rating
            height: 12
        }
    }
}
