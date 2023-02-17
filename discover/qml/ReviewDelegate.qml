/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.1
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

Kirigami.AbstractCard {
    id: reviewDelegateItem

    property bool compact: false
    property bool separator: true

    signal markUseful(bool useful)

    visible: model.shouldShow

    // Spacers to indent nested comments/replies
    Layout.leftMargin: depth * Kirigami.Units.largeSpacing

    contentItem: Item {
        implicitHeight: mainContent.childrenRect.height
        implicitWidth: mainContent.childrenRect.width
        ColumnLayout {
            id: mainContent
            anchors {
                left: parent.left
                top: parent.top
                right: parent.right
            }
            // Header with stars and date of review
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
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
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing

                elide: Text.ElideRight
                readonly property string author: reviewer ? reviewer : i18n("unknown reviewer")
                text: summary ? i18n("<b>%1</b> by %2", summary, author) : i18n("Comment by %1", author)
            }

            // Review text
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing

                text: display
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                text: packageVersion ? i18n("Version: %1", packageVersion) : i18n("Version: unknown")
                elide: Text.ElideRight
                opacity: 0.6
            }
        }
    }

    footer: Loader {
        active: !reviewDelegateItem.compact
        sourceComponent: RowLayout {
            id: rateTheReviewLayout
            visible: !reviewDelegateItem.compact
            Label {
                Layout.leftMargin: Kirigami.Units.largeSpacing
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
                    noButton.checked = false
                    reviewDelegateItem.markUseful(true)
                }
            }
            Button {
                id: noButton
                Layout.maximumWidth: Kirigami.Units.gridUnit * 3
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.bottomMargin: Kirigami.Units.smallSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                Layout.alignment: Qt.AlignVCenter

                text: i18nc("Keep this string as short as humanly possible", "No")

                checkable: true
                checked: usefulChoice === ReviewsModel.No
                onClicked: {
                    yesButton.checked = false
                    reviewDelegateItem.markUseful(false)
                }
            }
        }
    }
}
