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
import org.kde.discover 1.0

Item {
    id: shadow
    state: "thumbnail"
    property alias application: screenshotsModel.application
    readonly property rect initialGeometry: fullParent.flickableItem ?
        parent.mapFromItem(initialParent, fullParent.flickableItem.visibleArea.xPosition, fullParent.flickableItem.visibleArea.yPosition, initialParent.width, initialParent.height) :
        parent.mapFromItem(initialParent, 0, 0, initialParent.width, initialParent.height)
    readonly property rect fullGeometry: Qt.rect(0, 0, parent.width, parent.height)

    property Item initialParent: null
    property Item fullParent: null
    parent: fullParent

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
        readonly property real size: Math.min(parent.height, parent.width) * 0.9
        width: size
        height: size
        anchors.centerIn: parent
        running: visible
        visible: screenshot.status == Image.Loading
    }

    states: [
    State { name: "thumbnail"
        PropertyChanges { target: shadowItem; opacity: 0.1 }
        PropertyChanges { target: shadow; width: initialGeometry.width }
        PropertyChanges { target: shadow; height: initialGeometry.height }
        PropertyChanges { target: shadow; x: initialGeometry.x }
        PropertyChanges { target: shadow; y: initialGeometry.y }
        PropertyChanges { target: thumbnailsView; opacity: 1 }
    },
    State { name: "full"
        PropertyChanges { target: shadowItem; opacity: 0.7 }
        PropertyChanges { target: shadow; x: fullGeometry.x }
        PropertyChanges { target: shadow; y: fullGeometry.y }
        PropertyChanges { target: shadow; height: fullGeometry.height }
        PropertyChanges { target: shadow; width: fullGeometry.width }
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
