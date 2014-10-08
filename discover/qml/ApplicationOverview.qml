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
import QtQuick.Controls 1.1
import org.kde.muon.discover 1.0 as Discover
import org.kde.muon 1.0

Item {
    id: appInfo
    property QtObject application: null

    ScrollView {
        id: overviewContentsFlickable
        width: 2*parent.width/3
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        ApplicationDescription {
            width: overviewContentsFlickable.width-20
            height: childrenRect.height
            application: appInfo.application
        }
    }

    Column {
        anchors {
            top: parent.verticalCenter
            right: overviewContentsFlickable.left
            left: parent.left
            topMargin: 10
            margins: 5
        }
        spacing: 10
        InstallApplicationButton {
            id: installButton
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, maximumWidth)
            application: appInfo.application
            additionalItem:  Rating {
                property QtObject ratingInstance: application.rating
                visible: ratingInstance!=null
                rating:  ratingInstance==null ? 0 : ratingInstance.rating
            }
        }
        Grid {
            width: parent.width
            columns: 2
            spacing: 0
            Label { text: i18n("Total Size: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
            Label { text: application.sizeDescription; width: parent.width/2; elide: Text.ElideRight }
            Label { text: i18n("Version: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
            Label { text: application.packageName+" "+(application.isInstalled ? application.installedVersion : application.availableVersion); width: parent.width/2; elide: Text.ElideRight }
            Label { text: i18n("Homepage: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
            Label {
                text: application.homepage
                MouseArea {
                    anchors.fill: parent
                    onClicked: Qt.openUrlExternally(application.homepage);
                }
                SystemPalette { id: palette }
                color: palette.highlight
                font.underline: true
                width: parent.width/2
                elide: Text.ElideRight
            }
            Label { text: i18n("License: "); horizontalAlignment: Text.AlignRight; width: parent.width/2; font.weight: Font.Bold }
            Label { text: application.license; width: parent.width/2; elide: Text.ElideRight }
        }
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: application.isInstalled && application.canExecute
            text: i18n("Launch")
            onClicked: application.invokeApplication()
        }
    }
    
    Item {
        id: shadow
        state: "thumbnail"
        
        Rectangle {
            id: shadowItem
            anchors.fill: parent
            color: "black"
            Behavior on opacity { NumberAnimation { duration: 1000 } }
        }
        
        Image {
            id: screenshot
            anchors.centerIn: parent
            height: sourceSize ? Math.min(parent.height-5, sourceSize.height) : parent.height
            width: sourceSize ? Math.min(parent.width-5, sourceSize.width) : parent.width
            
            asynchronous: true
            fillMode: Image.PreserveAspectFit
            source: thumbnailsView.currentIndex>=0 ? screenshotsModel.screenshotAt(thumbnailsView.currentIndex) : "image://icon/image-missing"
            smooth: true
            visible: screenshot.status == Image.Ready
            
            onStatusChanged: if(status==Image.Error) {
                sourceSize.width = sourceSize.height = 200
                source="image://icon/image-missing"
            }
        }
        BusyIndicator {
            id: busy
            width: 128
            height: 128
            anchors.centerIn: parent
            running: visible
            visible: screenshot.status == Image.Loading
        }
        
        states: [
            State { name: "thumbnail"
                PropertyChanges { target: shadowItem; opacity: 0.1 }
                PropertyChanges { target: shadow; width: overviewContentsFlickable.x-x-5 }
                PropertyChanges { target: shadow; height: appInfo.height/2 }
                PropertyChanges { target: shadow; x: 5 }
                PropertyChanges { target: shadow; y: 5 }
                PropertyChanges { target: thumbnailsView; opacity: 1 }
            },
            State { name: "full"
                PropertyChanges { target: shadowItem; opacity: 0.7 }
                PropertyChanges { target: shadow; x: 0 }
                PropertyChanges { target: shadow; y: 0 }
                PropertyChanges { target: shadow; height: appInfo.height }
                PropertyChanges { target: shadow; width: appInfo.width }
                PropertyChanges { target: shadow; z: 0 }
                PropertyChanges { target: thumbnailsView; opacity: 0.3 }
            }
        ]
        transitions: Transition {
            SequentialAnimation {
                PropertyAction { target: screenshot; property: "smooth"; value: false }
                NumberAnimation { properties: "x,y,width,height"; easing.type: Easing.OutQuad; duration: 500 }
                PropertyAction { target: screenshot; property: "smooth"; value: true }
            }
        }
        
        MouseArea {
            anchors.fill: parent
            onClicked: { shadow.state = shadow.state == "thumbnail" ? "full" : "thumbnail" }
        }
        
        GridView {
            id: thumbnailsView
            cellHeight: 45
            cellWidth: 45
            interactive: false
            visible: screenshotsModel.count>1

            anchors {
                fill: shadow
                bottomMargin: 5
            }
            
            onCountChanged: currentIndex=0
            
            model: ScreenshotsModel {
                id: screenshotsModel
                application: appInfo.application
            }
            highlight: Rectangle { color: "white"; opacity: 0.5 }
            
            delegate: Image {
                source: small_image_url
                anchors.top: parent.top
                height: 40; width: 40
                fillMode: Image.PreserveAspectFit
                smooth: true
                MouseArea { anchors.fill: parent; onClicked: thumbnailsView.currentIndex=index}
            }
            Behavior on opacity { NumberAnimation { easing.type: Easing.OutQuad; duration: 500 } }
        }
    }
}
