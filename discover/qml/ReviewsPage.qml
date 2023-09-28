/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.14 as Kirigami

Kirigami.OverlaySheet {
    id: page
    parent: applicationWindow().overlay

    property alias model: reviewsView.model
    readonly property QtObject reviewsBackend: resource.backend.reviewsBackend
    readonly property var resource: model.resource

    readonly property var rd: ReviewDialog {
        id: reviewDialog

        application: page.resource
        backend: page.reviewsBackend
        onAccepted: backend.submitReview(resource, summary, review, rating, name)
    }

    function openReviewDialog() {
        page.sheetOpen = false
        reviewDialog.open()
    }

    header: ColumnLayout {
        width: parent.width
        spacing: Kirigami.Units.largeSpacing

        Kirigami.Heading {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: i18n("Reviews for %1", page.resource.name)
        }

        RowLayout {
            Layout.bottomMargin: Kirigami.Units.largeSpacing

            Button {
                id: reviewButton

                visible: page.reviewsBackend != null
                enabled: page.resource.isInstalled
                text: i18n("Write a Review…")
                onClicked: page.openReviewDialog()
            }
            Label {
                Layout.fillWidth: true
                text: i18n("Install this app to write a review")
                wrapMode: Text.WordWrap
                visible: !reviewButton.enabled
                opacity: 0.6
            }

        }
    }

    ListView {
        id: reviewsView

        clip: true
        topMargin: Kirigami.Units.largeSpacing
        leftMargin: Kirigami.Units.largeSpacing
        rightMargin: Kirigami.Units.largeSpacing
        bottomMargin: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.smallSpacing
        implicitWidth: Kirigami.Units.gridUnit * 25
        // Still preload some items to make the scrollbar behave better, but can't preload all the comments as some apps like Firefox have thousands of them which will freeze Discover for minutes
        cacheBuffer: height * 2
        reuseItems: true

        delegate: ReviewDelegate {
            width: ListView.view.width - ListView.view.leftMargin - ListView.view.rightMargin
            separator: index !== ListView.view.count - 1
            onMarkUseful: page.model.markUseful(index, useful)
        }
    }
}
