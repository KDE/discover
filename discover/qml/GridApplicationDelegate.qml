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

    topPadding: 0
    bottomPadding: 0

    contentItem: Item {
        implicitHeight: Kirigami.Units.gridUnit * 5
        Kirigami.Icon {
            id: icon
            anchors {
                left: parent.left
                top: parent.top
                bottom: parent.bottom
            }
            width: Kirigami.Units.gridUnit * 4
            source: model.application.icon
        }

        ColumnLayout {
            spacing: 0
            anchors {
                right: parent.right
                top: parent.top
                bottom: parent.bottom
                left: icon.right
                leftMargin: Kirigami.Units.largeSpacing
                topMargin: Kirigami.Units.smallSpacing
                bottomMargin: Kirigami.Units.smallSpacing
            }

            Kirigami.Heading {
                id: head
                level: delegateArea.compact ? 3 : 2
                type: Kirigami.Heading.Type.Primary
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignBottom
                wrapMode: Text.WordWrap
                maximumLineCount: 2

                text: model.application.name
            }

            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                maximumLineCount: 2
                opacity: 0.6
                wrapMode: Text.WordWrap

                text: model.application.comment
            }
        }
    }

    onClicked: Navigation.openApplication(model.application)
}
