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
import QtGraphicalEffects 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

GridItem {
    id: delegateRoot
    clip: true
    enabled: true
    onClicked: {
        Navigation.openApplication(application)
    }
    internalMargin: 0

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            id: artwork
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.darker(colors.dominantColor) }
                GradientStop { position: 0.5; color: "black" }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: parent.height*0.7
            readonly property bool hasThumbnail: screen.status != Image.Error && screen.source.toString()

            IconColors {
                id: colors
                iconName: model.application.icon
            }

            Image {
                id: screen
                anchors {
                    right: rating.right
                    rightMargin: (parent.width-width)/8
                    verticalCenter: parent.verticalCenter
                    verticalCenterOffset: -rating.height/3
                    bottom: rating.top
                }

                source: model.application.thumbnailUrl
                height: parent.height*0.8
                fillMode: Image.PreserveAspectFit
                smooth: false
                cache: false
                asynchronous: true
            }
            Image {
                id: icon

                height: parent.height*0.7
                width: height
                smooth: true
                asynchronous: true
                sourceSize: Qt.size(width, width)
                source: model.application.icon.length==0 ? ""
                    : model.application.icon[0] == "/" ? "file://"+model.application.icon
                    : "image://icon/"+model.application.icon
            }

            Rating {
                id: rating
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 3
                    right: parent.right
                    rightMargin: 3
                }
                starSize: parent.height*0.15
                rating: model.rating
            }

            state: artwork.hasThumbnail ? "withthumbnail" : "nothumbnail"
            states: [
                State {
                    name: "withthumbnail"

                    PropertyChanges {
                        target: icon
                        anchors {
                            left: artwork.left
                            leftMargin: artwork.width/6
                            verticalCenter: screen.verticalCenter
                        }
                    }
                },
                State {
                    name: "nothumbnail"

                    PropertyChanges {
                        target: icon
                        anchors {
                            centerIn: artwork
                            verticalCenter: artwork.verticalCenter
                        }
                    }
                }
            ]
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.rightMargin: 5
            ColumnLayout {
                spacing: 0
                Layout.fillWidth: true
                Layout.bottomMargin: 3
                Layout.topMargin: 3
                Layout.leftMargin: 5
                Layout.rightMargin: 5
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: name
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: comment
                    font: SystemFonts.smallestReadableFont
                    opacity: 0.6
                }
            }

            ConditionalLoader {
                Layout.minimumWidth: parent.width/3
                visible: condition
                condition: delegateRoot.containsMouse

                componentTrue: InstallApplicationButton {
                    application: model.application
                    canUpgrade: false
                }
            }
        }

        ApplicationIndicator {
            Layout.fillWidth: true
            id: indicator
            state: canUpgrade ? "upgradeable" : (isInstalled ? "installed" : "none")
            height: 5
        }
    }
}
