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
import org.kde.plasma.extras 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Item {
    id: appInfo
    property QtObject application: null
    property variant reviewsBackend: application.backend.reviewsBackend
    
    NativeScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: overviewContentsFlickable
        anchors {
            top: overviewContentsFlickable.top
            bottom: overviewContentsFlickable.bottom
            right: parent.right
        }
    }
    Flickable {
        clip: true
        id: overviewContentsFlickable
        width: 2*parent.width/3
        anchors {
            top: parent.top
            right: scroll.left
            bottom: parent.bottom
        }
        contentHeight: overviewContents.childrenRect.height
        Column {
            id: overviewContents
            width: parent.width
            spacing: 10
            
            property QtObject ratingInstance: appInfo.reviewsBackend!=null ? appInfo.reviewsBackend.ratingForApplication(appInfo.application) : null
            
            Item {
                id: intro
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: 10
                }
                height: Math.max(icon.height, header.height)+10
                IconItem {
                    id: icon
                    anchors.top: parent.top
                    anchors.left: parent.left
                    width: height
                    height: header.height
                    
                    source: application.icon
                }
                
                Column {
                    id: header
                    anchors {
                        top: parent.top
                        left: icon.right
                        right: parent.right
                        leftMargin: 5
                        topMargin: 5
                    }
                    
                    Heading {
                        text: application.name
                        width: parent.width
                        font.bold: true
                    }
                    Label {
                        anchors {
                            left: parent.left
                            right: parent.right
                        }
                        text: application.comment
                        wrapMode: Text.WordWrap
                    }
                    Rating {
                        visible: overviewContents.ratingInstance!=null
                        rating: overviewContents.ratingInstance == null ? 0 : overviewContents.ratingInstance.rating
                        width: 100
                    }
                }
                
                Row {
                    anchors {
                        right: parent.right
                        verticalCenter: parent.verticalCenter
                    }
                    spacing: 5
                    InstallApplicationButton {
                        id: installButton
                        width: maximumWidth
                        application: appInfo.application
                    }
                    Button {
                        visible: application.isInstalled && application.canExecute
                        text: i18n("Launch")
                        onClicked: application.invokeApplication()
                    }
                }
            }
            Heading { text: "Description" }
            Label {
                id: info
                anchors {
                    left: parent.left
                    right: parent.right
                    margins: 5
                }
                horizontalAlignment: Text.AlignJustify
                wrapMode: Text.WordWrap
                text: application.longDescription
            }
            
            AddonsView {
                id:addonsView
                application: appInfo.application
                isInstalling: installButton.isActive
                visible: count>0
                header: Heading { text: i18n("Addons") }
            }
            ListView {
                width: parent.width
                height: 123
                spacing: 5
                visible: count>0
                clip: true
                interactive: false
                
                header: Heading { text: i18n("Comments") }
                delegate: ReviewDelegate {
                    onMarkUseful: reviewsModel.markUseful(index, useful)
                }
                
                model: ReviewsModel {
                    id: reviewsModel
                    resource: application
                }
            }
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 5
                
                Button {
                    visible: reviewsModel.count>0
                    text: i18n("More comments (%1)...", overviewContents.ratingInstance ? overviewContents.ratingInstance.ratingCount() : 0)
                    onClicked: Navigation.openReviews(application, reviewsModel)
                }
                Button {
                    visible: appInfo.reviewsBackend != null && application.isInstalled
                    text: i18n("Review")
                    onClicked: reviewDialog.open()
                }
            }
        }
    }
    ReviewDialog {
        id: reviewDialog
        application: appInfo.application
        onAccepted: appInfo.reviewsBackend.submitReview(appInfo.application, summary, review, rating)
    }
    Column {
        anchors {
            top: parent.verticalCenter
            right: overviewContentsFlickable.left
            left: parent.left
            topMargin: 10
            margins: 5
        }
        Label {
            width: parent.width
            text: i18n(  "<b>Total Size:</b> %1<br/>"
                        +"<b>Version:</b> %2 %3<br/>"
                        +"<b>Homepage:</b> <a href='%4'>%4</a><br/>"
                        +"<b>License:</b> %5<br/>",
                        application.sizeDescription,
                        application.name, (application.isInstalled ?
                                                    application.installedVersion : application.availableVersion),
                        application.homepage,
                        application.license
            )
            onLinkActivated: Qt.openUrlExternally(link)
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
