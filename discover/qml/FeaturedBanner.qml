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
import "navigation.js" as Navigation

Information {
    id: info

    model: FeaturedModel {}
    
    delegate: MouseArea {
            property QtObject modelData: model
            enabled: modelData.package!=""
            width: 400
            height: 250
            
            onClicked: {
                if(modelData.packageName!=null)
                    Navigation.openApplication(ResourcesModel.resourceByPackageName(modelData.packageName))
                else
                    Qt.openUrlExternally(modelData.url)
            }
            
            z: PathView.isCurrentItem && !PathView.view.moving ? 1 : -1
            id: itemDelegate
            
            Loader {
                id: flick
                anchors.fill: parent

                function endsWith(str, suffix) {
                    return str.indexOf(suffix, str.length - str.length) !== -1;
                }

                source: endsWith(modelData.image, ".qml") ? modelData.image : "qrc:/qml/FeaturedImage.qml"

                Rectangle
                {
                    anchors.fill: parent
                    color: modelData.color
                    border.color: "cyan"
                }
            }

            Rectangle {
                anchors.fill: titleBar
                color: palette.midlight
                opacity: 0.7
                z: 20
            }

            SystemPalette { id: palette }

            RowLayout {
                id: titleBar
                height: description.paintedHeight*1.2
                z: 23
                spacing: 10
                property variant modelData: info.model.get(Math.min(info.currentIndex, info.model.count))
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }

                QIconItem {
                    height: parent.height
                    width: parent.height
                    icon: titleBar.modelData ? titleBar.modelData.icon : "kde"
                }

                Label {
                    id: description
                    Layout.fillWidth: true
                    anchors.verticalCenter: parent.verticalCenter

                    text: titleBar.modelData ? i18n("<b>%1</b><br/>%2", titleBar.modelData.text, titleBar.modelData.comment) : ""
                }
            }
        }
}
