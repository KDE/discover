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
import QtGraphicalEffects 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Information {
    id: info

    Connections {
        target: ResourcesModel
        onAllInitialized: featuredModel.initFeatured()
    }

    model: FeaturedModel {
        id: featuredModel
    }
    readonly property real side: Math.min(info.height*0.9, info.width/1.618)

    delegate: MouseArea {
            id: itemDelegate
            readonly property QtObject modelData: model
            readonly property int d1: Math.abs(info.currentIndex - model.index)
            readonly property int distance: Math.min(d1, info.count - d1)
            property real size: 1/((distance+3)/3)
            enabled: modelData.package!=""

            width: info.side*1.618 * size
            height: info.side * size

            Behavior on size {
                NumberAnimation {
                    duration: 500
                    easing.type: Easing.InQuad
                }
            }

            onClicked: {
                if(model.packageName !== "")
                    Navigation.openApplication(ResourcesModel.resourceByPackageName(modelData.packageName))
                else
                    Qt.openUrlExternally(modelData.url)
            }
            z: -itemDelegate.distance + PathView.isCurrentItem
            
            Loader {
                id: flick
                anchors.fill: parent

                function endsWith(str, suffix) {
                    return str.indexOf(suffix, str.length - suffix.length) !== -1;
                }

                source: modelData.image && endsWith(modelData.image, ".qml") ? modelData.image : "qrc:/qml/FeaturedImage.qml"
                clip: true
            }

            Rectangle {
                anchors.fill: titleBar
                color: DiscoverSystemPalette.midlight
                opacity: 0.7
                z: 20
            }

            RowLayout {
                id: titleBar
                height: description.implicitHeight
                z: 23
                spacing: 10
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

                QIconItem {
                    Layout.fillHeight: true
                    Layout.minimumWidth: height
                    icon: itemDelegate.modelData ? itemDelegate.modelData.icon : "kde"
                }

                ColumnLayout {
                    id: description
                    Layout.fillWidth: true

                    spacing: 0
                    Label {
                        Layout.fillWidth: true
                        text: itemDelegate.modelData.text
                        font.bold: true
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }

                    Label {
                        Layout.fillWidth: true
                        text: itemDelegate.modelData.comment
                        visible: text !== ""
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                }
            }

            DropShadow {
                anchors.fill: flick
                horizontalOffset: 3
                verticalOffset: 3
                radius: 8.0
                samples: 16
                color: "#80000000"
                source: flick
            }

            Desaturate {
                anchors.fill: flick
                source: flick
                desaturation: parent.PathView.isCurrentItem ? 0 : 0.8

                Behavior on desaturation {
                    NumberAnimation {
                        duration: 500
                        easing.type: Easing.InQuad
                    }
                }
            }
        }
}
