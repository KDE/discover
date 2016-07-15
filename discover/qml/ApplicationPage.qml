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
        RowLayout {
            Layout.fillWidth: true
            QIconItem {
                Layout.preferredHeight: 128
                Layout.preferredWidth: 128

                icon: appInfo.application.icon
                Layout.alignment: Qt.AlignTop
            }
            ColumnLayout {
                id: conts

                Layout.fillWidth: true
                Layout.fillHeight: true

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
                ApplicationDetails {
                    id: details
                    Layout.fillWidth: true
                    application: appInfo.application
                }
            }
        }

        ApplicationScreenshots {
            Layout.fillWidth: true
            resource: appInfo.application
        }

        ApplicationDescription {
            id: desc
            Layout.fillWidth: true

            application: appInfo.application
            z: -1
        }

        GridLayout {
            columns: 2
            Label { text: i18n("Homepage:") }
            ToolButton {
                text: application.homepage
                onClicked: Qt.openUrlExternally(application.homepage)
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignRight
            spacing: 5
            property QtObject rating: desc.application.rating

            Button {
                visible: parent.rating && parent.rating.ratingCount>0
                text: i18n("Show comments (%1)...", parent.rating ? parent.rating.ratingCount : 0)
                onClicked: Navigation.openReviews(application, reviewsModel)
            }
            Button {
                property QtObject reviewsBackend: application.backend.reviewsBackend
                visible: reviewsBackend != null && application.isInstalled
                text: i18n("Review")
                onClicked: reviewDialog.visible = true

                ReviewDialog {
                    id: reviewDialog
                    application: desc.application
                    onAccepted: application.backend.reviewsBackend.submitReview(application, summary, review, rating)
                }
            }
        }

        AddonsView {
            id: addonsView
            application: desc.application
            Layout.fillWidth: true
        }

        InstallApplicationButton {
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
    }
}
