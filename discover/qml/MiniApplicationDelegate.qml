/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2021 Carl Schwan <carlschwan@kde.org>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import "navigation.js" as Navigation
import org.kde.kirigami 2.6 as Kirigami

Kirigami.AbstractCard {
    id: delegateArea
    showClickFeedback: true

    contentItem: Item {
        implicitHeight: Kirigami.Units.gridUnit * 2

        Kirigami.Icon {
            id: resourceIcon
            source: model.applicationObject.icon
            height: Kirigami.Units.gridUnit * 3
            width: height
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
            }
        }

        ColumnLayout {
            spacing: 0
            anchors {
                verticalCenter: parent.verticalCenter
                right: parent.right
                left: resourceIcon.right
                leftMargin: Kirigami.Units.largeSpacing
            }

            Kirigami.Heading {
                id: head
                level: delegateArea.compact ? 3 : 2
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: model.applicationObject.name
                maximumLineCount: 1
            }

            Kirigami.Heading {
                id: category
                level: 5
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: model.applicationObject.comment
                maximumLineCount: 1
                opacity: 0.6
                visible: model.applicationObject.categoryDisplay && model.applicationObject.categoryDisplay !== page.title && !parent.bigTitle
            }

            RowLayout {
                spacing: Kirigami.Units.largeSpacing
                Layout.topMargin: Kirigami.Units.smallSpacing
                Layout.fillWidth: true
                Rating {
                    rating: model.applicationObject.rating ? model.applicationObject.rating.sortableRating : 0
                    starSize: head.font.pointSize
                }
                Label {
                    Layout.fillWidth: true
                    text: model.applicationObject.rating ? i18np("%1 rating", "%1 ratings", model.applicationObject.rating.ratingCount) : i18n("No ratings yet")
                    visible: model.applicationObject.rating || model.applicationObject.backend.reviewsBackend.isResourceSupported(model.applicationObject)
                    opacity: 0.5
                    elide: Text.ElideRight
                }
            }
        }
    }
}
