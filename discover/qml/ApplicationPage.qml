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
import QtQuick.Window 2.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami

Kirigami.ScrollablePage {
    id: appInfo
    property QtObject application: null
    clip: true

    readonly property var icon: application.icon
    title: application.name

    mainAction: Kirigami.Action { iconName: application.icon }

    ColumnLayout {
        ColumnLayout {
            id: conts
            anchors.fill: parent

            RowLayout {
                Layout.fillWidth: true
                QIconItem {
                    Layout.preferredHeight: title.height
                    Layout.preferredWidth: title.height

                    icon: appInfo.application.icon
                    Layout.alignment: Qt.AlignTop
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Heading {
                        id: title
                        text: appInfo.application.name
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Label {
                        Layout.fillWidth: true
                        text: appInfo.application.comment
                        wrapMode: Text.WordWrap
                        elide: Text.ElideRight
                        maximumLineCount: 1
                    }
                    Rating {
                        readonly property QtObject ratingInstance: application.rating
                        visible: ratingInstance!=null
                        rating:  ratingInstance==null ? 0 : ratingInstance.rating
                        starSize: title.paintedHeight

                        Text { text: parent.ratingInstance ? i18n(" (%1)", parent.ratingInstance.ratingCount) : "" }
                    }
                }
            }
            InstallApplicationButton {
                Layout.fillWidth: true
                application: appInfo.application
                fill: true
                additionalItem: Button {
                    Layout.fillWidth: true
                    visible: application.isInstalled && application.canExecute
                    text: i18n("Launch")
                    onClicked: application.invokeApplication()
                }
            }
        }

        Item {
            id: screenshotsPlaceholder
            Layout.fillWidth: true
            Layout.minimumHeight: width/1.618

            ApplicationScreenshots {
                application: appInfo.application
                initialParent: screenshotsPlaceholder
                fullParent: appInfo
            }
        }

        ApplicationDescription {
            id: desc
            Layout.fillWidth: true

            application: appInfo.application
            z: -1
        }

        ApplicationDetails {
            id: details
            Layout.fillWidth: true
            application: appInfo.application
        }
    }
}
