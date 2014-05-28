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
import org.kde.plasma.core 2.0
import org.kde.plasma.components 2.0
import org.kde.plasma.extras 2.0
import org.kde.muon 1.0
import org.kde.muon.discover 1.0 as Discover
import "navigation.js" as Navigation

Column {
    Item {width: 10; height: 5}

    Item {
        anchors {
            left: parent.left
            right: parent.right
            margins: 10
        }
        height: icon.height
        IconItem {
            id: icon
            anchors {
                top: header.top
                left: parent.left
            }
            height: 64
            width: height

            source: application.icon
            clip: true
        }

        Item {
            id: header
            height: parent.height
            anchors {
                top: parent.top
                left: icon.right
                right: parent.right
                leftMargin: 5
            }

            Heading {
                id: heading
                text: application.name
                width: parent.width
                elide: Text.ElideRight
                font.bold: true
            }
            Label {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: heading.bottom
                    bottom: parent.bottom
                }
                text: application.comment
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
                //                         verticalAlignment: Text.AlignVCenter
            }
        }
    }
    Item {width: 10; height: 20}
    Heading { text: i18n("Description") }
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
    Item {width: 10; height: 20}

    Heading {
        text: i18n("Addons")
        visible: addonsView.visible
    }
    AddonsView {
        id: addonsView
        application: appInfo.application
        isInstalling: installButton.isActive
        width: parent.width
    }

    Item {width: 10; height: 20}
    Heading {
        text: i18n("Comments")
        visible: reviewsView.visible
    }
    Repeater {
        id: reviewsView
        width: parent.width
        visible: count>0
        clip: true

        delegate: ReviewDelegate {
            onMarkUseful: reviewsModel.markUseful(index, useful)
        }

        model: Discover.PaginateModel {
            pageSize: 3
            sourceModel: ReviewsModel {
                id: reviewsModel
                resource: application
            }
        }
    }
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 5

        Button {
            visible: reviewsModel.count>0
            text: i18n("More comments (%1)...", appInfo.ratingInstance ? appInfo.ratingInstance.ratingCount() : 0)
            onClicked: Navigation.openReviews(application, reviewsModel)
        }
        Button {
            visible: appInfo.reviewsBackend != null && application.isInstalled
            text: i18n("Review")
            onClicked: reviewDialog.open()
        }
    }
    Item { height: 10; width: 5 } //margin by the end
}
