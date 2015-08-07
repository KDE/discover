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
import org.kde.kquickcontrolsaddons 2.0
import QtQuick.Window 2.1
import "navigation.js" as Navigation

ScrollView {
    id: parentItem
    property alias count: view.count
    property alias header: view.header
    property alias section: view.section
    property alias model: view.model
    readonly property real proposedMargin: app.isCompact ? 0 : (width-app.actualWidth)/2

    ListView
    {
        id: view
        spacing: 3
        snapMode: ListView.SnapToItem
        currentIndex: -1
        
        delegate: GridItem {
                id: delegateArea
//                 checked: view.currentIndex==index
                width: app.actualWidth
                x: parentItem.proposedMargin
                property real contHeight: height*0.8
                height: nameLabel.font.pixelSize*3
                internalMargin: 0

                onClicked: {
                    view.currentIndex = index
                    Navigation.openApplication(application)
                }

                QIconItem {
                    id: resourceIcon
                    icon: model.icon
                    width: contHeight
                    height: contHeight
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                    }

                    QIconItem {
                        anchors {
                            right: resourceIcon.right
                            bottom: resourceIcon.bottom
                        }
                        visible: isInstalled && view.model.stateFilter!=2
                        icon: "checkmark"
                        height: 16
                        width: 16
                    }
                }

                Label {
                    id: nameLabel
                    anchors {
                        top: resourceIcon.top
                        left: resourceIcon.right
                        right: ratingsItem.left
                        leftMargin: 5
                    }
                    font.pointSize: commentLabel.font.pointSize*1.7
                    elide: Text.ElideRight
                    text: name
                }
                Rating {
                    id: ratingsItem
                    anchors {
                        right: parent.right
                        verticalCenter: nameLabel.verticalCenter
                    }
                    height: app.isCompact ? contHeight*.6 : contHeight*.4
                    width: parent.width/5
                    rating: model.rating
                }

                RowLayout {
                    anchors {
                        bottom: parent.bottom
                        left: resourceIcon.right
                        leftMargin: 5
                        right: parent.right
                        top: nameLabel.bottom
                    }

                    Label {
                        id: commentLabel
                        Layout.fillWidth: true

                        elide: Text.ElideRight
                        text: comment
                        font.italic: true
                        opacity: delegateArea.containsMouse ? 1 : 0.2
                        maximumLineCount: 1
                        clip: true
                    }

                    Label {
                        visible: app.isCompact
                        text: model.application.status
                    }
                    Text {
                        Layout.fillHeight: true
                        width: 5
                        text: parent.height
                        Rectangle { color: "red"; anchors.fill: parent; opacity: 0.3 }
                    }
                    InstallApplicationButton {
                        Layout.maximumHeight: parent.height
    //                     property bool isVisible: delegateArea.containsMouse && !canHide
    //                     opacity: isVisible ? 1 : 0
                        application: model.application
                    }
                }
            }
    }
}
