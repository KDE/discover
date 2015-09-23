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
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.kquickcontrolsaddons 2.0
import "navigation.js" as Navigation

GridItem {
    id: delegateRoot
    clip: true
    property bool allInstalled: false
    enabled: true
    onClicked: {
        Navigation.openApplication(application)
    }
    internalMargin: 0

    SystemPalette { id: sys }
    Rectangle {
        id: artwork
        color: sys.shadow
        width: parent.width
        height: parent.height*0.7

        ConditionalLoader {
            id: artworkConditional
            anchors.fill: parent

            condition: model.application.thumbnailUrl!=""
            componentTrue: Item {
                Image {
                    id: screen
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                        rightMargin: parent.width*0.1
                    }

                    source: model.application.thumbnailUrl
                    height: parent.height*0.9
                    fillMode: Image.PreserveAspectFit
                    smooth: false
                    cache: false
                    asynchronous: true
                    onStatusChanged:  {
                        if(status==Image.Error) {
                            artworkConditional.hasThumbnail=false
                        }
                    }
                }
                Image {
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: parent.width*0.1
                    }

                    id: smallIcon
                    width: 64
                    height: width
                    smooth: true
                    asynchronous: true
                    source: model.application.icon[0] == "/" ? "file://"+model.application.icon : "image://icon/"+model.application.icon
                }
            }
            componentFalse: Item {
                Image {
                    anchors.centerIn: parent
                    height: parent.height*0.7
                    width: height
                    smooth: true
                    asynchronous: true
                    source: model.application.icon[0] == "/" ? "file://"+model.application.icon : "image://icon/"+model.application.icon
                }
            }
        }
    }
    RowLayout {
        anchors {
            topMargin: artwork.height+2
            fill: parent
        }

        ColumnLayout {
            Layout.fillWidth: true
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: name
            }
            Label {
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: comment
                opacity: 0.6
            }
        }
        ConditionalLoader {
            Layout.minimumWidth: parent.width/3
            Layout.fillHeight: true

            condition: delegateRoot.containsMouse
            componentFalse: Rating {
                rating: 5
            }

            componentTrue: InstallApplicationButton {
                application: model.application
                canUpgrade: false
            }
        }
    }

    Rectangle {
        id: indicator
        color: canUpgrade ? "#55f" : (isInstalled ? "#5f5" : "transparent")
        height: 5
        width: parent.width
        anchors.bottom: parent.bottom
        opacity: 0.7
    }
}
