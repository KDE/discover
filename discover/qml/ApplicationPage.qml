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
import QtGraphicalEffects 1.0
import org.kde.kquickcontrolsaddons 2.0
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

DiscoverPage {
    id: appInfo
    property QtObject application: null
    clip: true

    title: application.name

    background: Rectangle { color: Kirigami.Theme.viewBackgroundColor }

    ReviewsPage {
        id: reviewsSheet
        model: reviewsModel
    }

    pageOverlay: Item {
        InstallApplicationButton {
            id: button
            anchors {
                right: parent.right
                bottom: parent.bottom
                bottomMargin: Kirigami.Units.gridUnit
                rightMargin: appInfo.width - appInfo.flickable.width
            }

            Layout.alignment: Qt.AlignRight
            application: appInfo.application
            fill: true
            additionalItem: Button {
                Layout.fillWidth: true
                visible: application.isInstalled && application.canExecute
                text: i18n("Launch")
                onClicked: application.invokeApplication()
            }
        }

        DropShadow {
            anchors.fill: button
            source: button
            horizontalOffset: 2
            verticalOffset: 2
            radius: 12.0
            samples: 17
            color: "gray"
        }
    }

    ColumnLayout {
        RowLayout {
            Layout.fillWidth: true
            QIconItem {
                Layout.preferredHeight: 128
                Layout.preferredWidth: 128

                icon: appInfo.application.icon
                Layout.alignment: Qt.AlignVCenter
            }
            ColumnLayout {
                id: conts

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.largeSpacing
                spacing: 0

                Heading {
                    id: title
                    text: appInfo.application.name
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }
                Rectangle {
                    color: Kirigami.Theme.linkColor
                    Layout.fillWidth: true
                    height: 1
                }
                Label {
                    Layout.fillWidth: true
                    text: appInfo.application.comment
                    wrapMode: Text.WordWrap
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
                Label {
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: appInfo.application.categoryDisplay
                    color: Kirigami.Theme.linkColor
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
                Label {
                    text: i18n("Version: %1", appInfo.application.isInstalled ? appInfo.application.installedVersion : appInfo.application.availableVersion)
                }
                Label {
                    text: i18n("Size: %1", appInfo.application.sizeDescription)
                }
                Label {
                    visible: text.length>0
                    text: appInfo.application.license ? i18n("License: %1", appInfo.application.license) : ""
                }
            }
        }

        ApplicationScreenshots {
            Layout.fillWidth: true
            resource: appInfo.application
            page: appInfo
        }

        Heading {
            text: i18n("Description")
            Layout.fillWidth: true
            visible: appInfo.application.longDescription.length > 0
        }
        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignJustify
            wrapMode: Text.WordWrap
            text: appInfo.application.longDescription
        }

        RowLayout {
            visible: button.text.length > 0
            Label {
                text: i18n("Homepage: ")
            }
            LinkButton {
                id: button
                text: application.homepage
                onClicked: Qt.openUrlExternally(application.homepage)
            }
        }

        LinkButton {
            id: addonsButton
            text: i18n("Addons")
            visible: addonsView.containsAddons
            onClicked: addonsView.opened = true
        }

        LinkButton {
            readonly property QtObject rating: appInfo.application.rating
            visible: rating && rating.ratingCount>0 && reviewsModel.count
            text: i18n("Show comments (%1)...", rating ? rating.ratingCount : 0)

            ReviewsModel {
                id: reviewsModel
                resource: appInfo.application
            }

            onClicked: {
                reviewsSheet.open()
            }
        }

        Item {
            height: addonsButton.height
            width: 5
        }
    }

    readonly property var addons: AddonsView {
        id: addonsView
        application: appInfo.application
        parent: overlay
    }
}
