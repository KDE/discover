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

import QtQuick 1.1
import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation

Item {
    id: parentItem
    property alias count: view.count
    property alias header: view.header
    property alias section: view.section
    property alias model: view.model
    property real actualWidth: width
    property real proposedMargin: (view.width-actualWidth)/2

    ListView
    {
        id: view
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
        }
        spacing: 3
        snapMode: ListView.SnapToItem
        currentIndex: -1
        
        delegate: ListItem {
                checked: view.currentIndex==index
                width: parentItem.actualWidth
                x: parentItem.proposedMargin
                property real contHeight: height*0.8
                height: nameLabel.font.pixelSize*3
                MouseArea {
                    id: delegateArea
                    anchors.fill: parent
                    onClicked: {
                        view.currentIndex = index
                        Navigation.openApplication(application)
                    }
                    hoverEnabled: true
                    IconItem {
                        id: icon
                        source: model.icon
                        width: contHeight
                        height: contHeight
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                    }
                    
                    IconItem {
                        anchors.right: icon.right
                        anchors.bottom: icon.bottom
                        visible: isInstalled && view.model.stateFilter!=2
                        source: "dialog-ok"
                        height: 16
                        width: 16
                    }
                    Label {
                        id: nameLabel
                        anchors.top: icon.top
                        anchors.left: icon.right
                        anchors.right: ratingsItem.left
                        anchors.leftMargin: 5
                        font.pointSize: commentLabel.font.pointSize*1.7
                        elide: Text.ElideRight
                        text: name
                    }
                    Label {
                        id: commentLabel
                        anchors {
                            bottom: icon.bottom
                            left: icon.right
                            leftMargin: 5
                            right: installButton.left
                        }
                        elide: Text.ElideRight
                        text: comment
                        font.italic: true
                        opacity: delegateArea.containsMouse ? 1 : 0.2
                    }
                    Rating {
                        id: ratingsItem
                        anchors {
                            right: parent.right
                            top: parent.top
                        }
                        height: contHeight*.4
                        rating: model.rating
                    }
                    
                    InstallApplicationButton {
                        id: installButton
                        anchors {
                            bottom: parent.bottom
                            right: parent.right
                        }
                        width: ratingsItem.width*2
//                         property bool isVisible: delegateArea.containsMouse && !installButton.canHide
//                         opacity: isVisible ? 1 : 0
                        application: model.application
                    }
                }
            }
    }
    
    NativeScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
        }
    }
}
