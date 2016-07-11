/*
 *   Copyright (C) 2012-2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

ColumnLayout
{
    id: page
    property alias category: catModel.displayedCategory
    property real spacing: 3
    property real maxtopwidth: 250
    property alias search: bread.search

    PageHeader {
        Layout.fillWidth: true
        background: category ? category.decoration : "https://c2.staticflickr.com/8/7193/6900377481_76367f973a_o.jpg"

        headerItem: Breadcrumbs {
            id: bread
            category: page.category
        }

        ListView {
            Layout.fillWidth: true
            orientation: ListView.Horizontal
            model: CategoryModel {
                id: catModel
            }
            delegate: ToolButton {
                text: display
                onClicked: Navigation.openCategory(category)
            }
            spacing: 3
            height: SystemFonts.generalFont.pointSize * 3
        }
    }

    RowLayout {
        id: gridRow
        readonly property bool extended: top.count>5
        spacing: page.spacing

        ApplicationsTop {
            id: top
            Layout.fillHeight: true
            Layout.fillWidth: true
            sortRole: "ratingCount"
            filteredCategory: page.category
            title: i18n("Most Popular")
            extended: gridRow.extended
            roleDelegate: Item {
                width: bg.width
                implicitWidth: bg.implicitWidth
                property var model
                LabelBackground {
                    id: bg
                    anchors.centerIn: parent
                    text: model ? model.ratingCount : ""
                }
            }
            Layout.preferredWidth: page.maxtopwidth
        }
        ApplicationsTop {
            id: top2
            Layout.preferredWidth: page.maxtopwidth
            Layout.fillHeight: true
            Layout.fillWidth: true
            sortRole: "ratingPoints"
            filteredCategory: page.category
            title: i18n("Best Rating")
            extended: gridRow.extended
            roleDelegate: Rating {
                property var model
                rating: model ? model.rating : 0
                starSize: parent.height/3
            }
        }
    }
}
