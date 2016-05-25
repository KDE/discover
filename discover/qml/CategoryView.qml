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
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

GridItem
{
    property alias category: categoryModel.displayedCategory
    readonly property alias count: grid.count
    supportsMouseEvents: false

    ScrollView {
        anchors {
            margins: -3
            fill: parent
        }
        horizontalScrollBarPolicy: Qt.ScrollBarAlwaysOff
        verticalScrollBarPolicy: Qt.ScrollBarAlwaysOn

        GridView {
            id: grid
            readonly property real iconSide: 32

            cellWidth: Helpers.isCompact ? width : width/Math.floor(width/100)
            cellHeight: Helpers.isCompact ? 35 : (grid.iconSide + SystemFonts.generalFont.pixelSize*3 + 5)
            boundsBehavior: Flickable.StopAtBounds
            footer: Grid {
                Repeater {
                    model: CategoryModel {
                        displayedCategory: categoryModel.displayedCategory
                        filter: CategoryModel.OnlyAddons
                    }
                    delegate: categoryIconDelegate
                }
            }
            header: Item { height: 10; width: 10 }
            model: CategoryModel {
                id: categoryModel
                filter: CategoryModel.NoAddons
            }

            delegate: categoryIconDelegate
        }
    }

    Component {
        id: categoryIconDelegate
        MouseArea {
            id: categoryItem
            enabled: true

            width: grid.cellWidth
            height: grid.cellHeight-2
            hoverEnabled: true

            ColumnLayout {
                id: layout

                anchors.top: parent.top
                width: parent.width
                Item {
                    Layout.fillWidth: true
                    Layout.preferredWidth: grid.iconSide
                    Layout.preferredHeight: Layout.preferredWidth
                    opacity: categoryItem.containsMouse ? 0.5 : 1

                    QIconItem {
                        icon: decoration
                        width: grid.iconSide
                        height: width
                        anchors.centerIn: parent
                    }
                }
                Label {
                    id: nameLabel
                    text: display
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    wrapMode: Text.WordWrap

                    maximumLineCount: 2
                }
            }
            onClicked: Navigation.openCategory(category)
        }
    }
}
