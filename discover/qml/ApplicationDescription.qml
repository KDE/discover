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
import org.kde.discover 1.0
import org.kde.discover.app 1.0 as Discover
import "navigation.js" as Navigation

ColumnLayout
{
    id: desc
    property QtObject application: null
    property alias isInstalling: addonsView.isInstalling

    readonly property real margin: Discover.SystemFonts.generalFont.pointSize

    Heading {
        text: i18n("Description")
    }
    Label {
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignJustify
        wrapMode: Text.WordWrap
        text: application.longDescription
    }
    Item { width: 1; height: parent.margin }

    AddonsView {
        id: addonsView
        application: parent.application
        Layout.fillWidth: true
    }

    Item { width: 1; height: parent.margin }
    Heading {
        text: i18n("Comments")
        visible: reviewsView.visible
    }
    Repeater {
        id: reviewsView
        visible: count>0

        delegate: ReviewDelegate {
            Layout.fillWidth: true
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
        property QtObject rating: desc.application.rating

        Button {
            visible: reviewsView.visible
            text: i18n("More comments (%1)...", parent.rating ? parent.rating.ratingCount : 0)
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
    Item { width: 1; height: parent.margin/2 }
}
