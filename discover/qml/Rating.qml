/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.14 as Kirigami

RowLayout {
    id: view

    property bool editable: false
    property int max: 10
    property int rating: 0
    property real starSize: Kirigami.Units.gridUnit

    clip: true
    spacing: 0

    readonly property var ratingIndex: (theRepeater.count / max) * rating

    Repeater {
        id: theRepeater
        model: 5
        delegate: Kirigami.Icon {
            Layout.minimumWidth: view.starSize
            Layout.minimumHeight: view.starSize
            Layout.preferredWidth: view.starSize
            Layout.preferredHeight: view.starSize

            width: height
            source: "rating"
            opacity: (view.editable && mouse.item.containsMouse ? 0.7
                        : index >= view.ratingIndex ? 0.2
                        : 1)

            ConditionalLoader {
                id: mouse

                anchors.fill: parent
                condition: view.editable
                componentTrue: MouseArea {
                    hoverEnabled: true
                    onClicked: rating = (view.max / theRepeater.count * (index + 1))
                }
                componentFalse: null
            }
        }
    }
}
