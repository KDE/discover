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
import org.kde.muon 1.0
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

ConditionalLoader
{
    id: page
    property QtObject category: null

    CategoryModel {
        id: categoryModel
        displayedCategory: page.category
    }

    condition: !app.isCompact
    componentTrue: RowLayout {
            id: gridRow
            readonly property bool extended: !app.isCompact && view.count>5
            spacing: 3

            ApplicationsTop {
                id: top
                Layout.fillHeight: true
                Layout.fillWidth: true
                sortRole: "sortableRating"
                filteredCategory: categoryModel.displayedCategory
                title: i18n("Popularity")
                extended: gridRow.extended
                roleDelegate: Item {
                    width: bg.width
                    property variant model
                    LabelBackground {
                        id: bg
                        anchors.centerIn: parent
                        text: model.sortableRating.toFixed(2)
                    }
                }
            }
            ApplicationsTop {
                id: top2
                Layout.fillHeight: true
                Layout.fillWidth: true
                sortRole: "ratingPoints"
                filteredCategory: categoryModel.displayedCategory
                title: i18n("Rating")
                extended: gridRow.extended
                roleDelegate: Rating {
                    property variant model
                    rating: model.rating
                    height: 12
                }
            }

            Item {
                width: 4
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: page.width/2
                Layout.maximumHeight: top.height

                spacing: -1

                Label {
                    text: i18n("Categories")
                    Layout.fillWidth: true
                    font.weight: Font.Bold
                    Layout.minimumHeight: paintedHeight*1.5
                }

                CategoryView {
                    id: view
                    model: categoryModel
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }

    componentFalse: ColumnLayout {
            Layout.minimumHeight: 5000

            ApplicationsTop {
                id: top
                Layout.fillHeight: true
                Layout.fillWidth: true
                sortRole: "sortableRating"
                filteredCategory: categoryModel.displayedCategory
                title: i18n("Popularity")
                roleDelegate: Item {
                    width: bg.width
                    property variant model
                    LabelBackground {
                        id: bg
                        anchors.centerIn: parent
                        text: model.sortableRating.toFixed(2)
                    }
                }
            }
            Label {
                text: i18n("Categories")
                Layout.fillWidth: true
                font.weight: Font.Bold
                Layout.minimumHeight: paintedHeight*1.5
            }

            Repeater {
                id: view
                Layout.fillWidth: true
                model: categoryModel

                delegate: GridItem {
                    height: 40
                    Layout.fillWidth: true
                    RowLayout {
                        QIconItem {
                            icon: decoration
                            width: 32
                            height: width
                        }
                        Label {
                            text: display
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap

                            maximumLineCount: 2
                        }
                    }
                    onClicked: Navigation.openCategory(category)
                }
            }
    }
}
