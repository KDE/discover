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
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1

Item {
    id: appInfo
    property QtObject application: null
    clip: true

    ConditionalLoader {
        anchors.fill: parent
        condition: app.isCompact

        componentFalse: Item {
            ScrollView {
                id: overviewContentsFlickable
                width: 2*parent.width/3
                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    right: parent.right
                }
                ApplicationDescription {
                    width: overviewContentsFlickable.viewport.width
                    application: appInfo.application
                }
            }

            ApplicationDetails {
                anchors {
                    top: parent.verticalCenter
                    right: overviewContentsFlickable.left
                    left: parent.left
                    topMargin: 10
                    margins: 5
                }
                application: appInfo.application
            }

            ApplicationScreenshots {
                application: appInfo.application
                initialGeometry: Qt.rect(5,5, overviewContentsFlickable.x-10, appInfo.height/2)
            }
        }
        componentTrue: ScrollView {
            id: scroll
            ColumnLayout {
                width: scroll.viewport.width

                ApplicationDetails {
                    Layout.fillWidth: true
                    application: appInfo.application
                }

                Item {
                    Layout.fillWidth: true
                    Layout.minimumHeight: 100

                    ApplicationScreenshots {
                        application: appInfo.application
                        initialGeometry: Qt.rect(0, 0, parent.width, parent.height)
                        fullGeometry: Qt.rect(-parent.x, -parent.y, scroll.viewport.width, scroll.viewport.height)
                    }
                }

                ApplicationDescription {
                    Layout.fillWidth: true

                    application: appInfo.application
                    z: -1
                }
            }
        }
    }
}
