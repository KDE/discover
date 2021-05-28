/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
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
        parent: applicationWindow().overlay

        application: page.resource
        backend: page.reviewsBackend
        onAccepted: backend.submitReview(resource, summary, review, rating)
    }

    function openReviewDialog() {
        reviewDialog.sheetOpen = true
        page.sheetOpen = false
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
        topMargin: 0
        leftMargin: 0
        rightMargin: 0
        bottomMargin: 0
        spacing: 0
        implicitWidth: Kirigami.Units.gridUnit * 25
        cacheBuffer: Math.max(0, contentHeight)

        delegate: Kirigami.AbstractListItem {
            width: reviewsView.width
            highlighted: false
            padding: Kirigami.Units.largeSpacing
            contentItem: ColumnLayout {
                // Header with stars and date of review
                RowLayout {
                    Layout.fillWidth: true
                    Rating {
                        id: rating
                        Layout.fillWidth: true
                        Layout.bottomMargin: Kirigami.Units.largeSpacing
                        rating: model.rating
                        starSize: Kirigami.Units.gridUnit
                    }
                    Label {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignRight
                        text: date.toLocaleDateString(Qt.locale(), "MMMM yyyy")
                        opacity: 0.6
                    }
                }

                // Review title and author
                Label {
                    id: content
                    Layout.fillWidth: true

                    elide: Text.ElideRight
                    readonly property string author: reviewer ? reviewer : i18n("unknown reviewer")
                    text: summary ? i18n("<b>%1</b> by %2", summary, author) : i18n("Comment by %1", author)
                }

                // Review text
                Label {
                    Layout.fillWidth: true
                    Layout.bottomMargin: Kirigami.Units.smallSpacing

                    text: display
                    wrapMode: Text.Wrap
                }

                Label {
                    Layout.fillWidth: true
                    text: packageVersion ? i18n("Version: %1", packageVersion) : i18n("Version: unknown")
                    elide: Text.ElideRight
                    opacity: 0.6
                }

                // Vote
                RowLayout {
                    id: rateTheReviewLayout
                    Label {
                        visible: usefulnessTotal !== 0
                        text: i18n("Votes: %1 out of %2", usefulnessFavorable, usefulnessTotal)
                    }

                    Label {
                        Layout.fillWidth: true
                        Layout.leftMargin: usefulnessTotal === 0 ? Kirigami.Units.largeSpacing : 0
                        horizontalAlignment: Text.AlignRight
                        text: i18n("Was this review useful?")
                        elide: Text.ElideLeft
                    }

                    // Useful/Not Useful buttons
                    Button {
                        id: yesButton
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.bottomMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        text: i18nc("Keep this string as short as humanly possible", "Yes")

                        checkable: true
                        checked: usefulChoice === ReviewsModel.Yes
                        onClicked: {
                            noButton.checked = false;
                            page.model.markUseful(index, true);
                        }
                    }
                    Button {
                        id: noButton
                        Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                        Layout.topMargin: Kirigami.Units.smallSpacing
                        Layout.bottomMargin: Kirigami.Units.smallSpacing
                        Layout.alignment: Qt.AlignVCenter

                        text: i18nc("Keep this string as short as humanly possible", "No")

                        checkable: true
                        checked: usefulChoice === ReviewsModel.No
                        onClicked: {
                            yesButton.checked = false;
                            page.model.markUseful(index, false);
                        }
                    }
                }
            }
        }
    }
}
