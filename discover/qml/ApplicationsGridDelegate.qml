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

import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation
import QtQuick 1.1

GridItem {
    id: delegateRoot
    clip: true
    property bool requireClick: false
    property bool displayDescription: false
    enabled: true
    onClicked: {
        if(delegateRoot.requireClick && !displayDescription) {
            displayDescription=true
        } else
            Navigation.openApplication(application)
    }
    Timer {
        id: timer
        interval: 200
        onTriggered: delegateRoot.displayDescription=true
    }
    onPositionChanged: if(!delegateRoot.requireClick) timer.restart()
    onContainsMouseChanged: {
        if(!containsMouse) {
            if(!delegateRoot.requireClick)
                timer.stop();
            displayDescription=false
        }
    }
    onDisplayDescriptionChanged: {
        if(delegateRoot.displayDescription)
            descriptionLoader.sourceComponent=extraInfoComponent
    }

    Flickable {
        id: delegateFlickable
        anchors.fill: parent
        contentHeight: delegateRoot.height*2
        interactive: false
        contentY: delegateRoot.displayDescription ? delegateRoot.height : 0
        Behavior on contentY { NumberAnimation { duration: 200; easing.type: Easing.InQuad } }
        
        Image {
            id: screen
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: parent.top
                topMargin: 5
            }
            property bool hasThumbnail: model.application.thumbnailUrl!=""
            source: hasThumbnail ? model.application.thumbnailUrl : "image://icon/"+model.application.icon
            height: delegateRoot.height*0.7
            fillMode: Image.PreserveAspectFit
            smooth: false
            cache: false
            asynchronous: true
            onStatusChanged:  {
                if(status==Image.Error) {
                    hasThumbnail=false
                }
            }
        }
        Image {
            id: smallIcon
            anchors {
                right: parent.right
                rightMargin: 5
            }
            y: 5+(delegateRoot.displayDescription ? delegateRoot.height : screen.height-height )
            width: 48
            height: width
            smooth: true
            asynchronous: true
            source: "image://icon/"+model.application.icon
            visible: screen.hasThumbnail
            Behavior on y { NumberAnimation { duration: 200; easing.type: Easing.InQuad } }
        }
        Label {
            anchors {
                top: screen.bottom
                left: parent.left
                right: parent.right
                leftMargin: 5
            }
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: name
        }
        Loader {
            id: descriptionLoader
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                top: parent.verticalCenter
                bottomMargin: 10
            }
            clip: true
        }
    }
    
    Component {
        id: extraInfoComponent
        Item {
            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    bottom: installButton.top
                    rightMargin: smallIcon.visible ? 48 : 0
                    topMargin: 5
                }
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: model.application.comment
            }
            InstallApplicationButton {
                id: installButton
                height: 30
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                    bottomMargin: 5
                }
                application: model.application
                additionalItem: Rating {
                    rating: model.rating
                    visible: !model.application.canUpgrade && model.rating>=0
                }
            }
        }
    }
}
