/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover.app 1.0
import org.kde.kirigami 2.0 as Kirigami

Control
{
    id: root
    property alias text: theLabel.text
    property real progress: 1.
    padding: Kirigami.Units.smallSpacing * 1.5

    background: Item {
        Rectangle {
            color: Kirigami.Theme.disabledTextColor
            anchors.fill: parent
            radius: root.padding
        }

        Rectangle {
            anchors {
                fill: parent
                rightMargin: (1-root.progress) * parent.width
            }
            color: Kirigami.Theme.highlightColor
            radius: root.padding
        }
    }

    contentItem: Label {
        id: theLabel
        horizontalAlignment: Text.AlignHCenter
        color: Kirigami.Theme.highlightedTextColor
    }

    ToolTip.visible: hovered
    ToolTip.text: theLabel.text
}
