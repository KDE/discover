/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover.app 1.0
import org.kde.kirigami 2.14 as Kirigami

Control
{
    id: root
    property alias text: theLabel.text
    property real progress: 1.0
    readonly property bool inProgress: progress > 0
    padding: Kirigami.Units.smallSpacing * 1.5

    background: Item {
        visible: root.inProgress
        Rectangle {
            color: Kirigami.Theme.disabledTextColor
            border.width: 1
            border.color: Qt.darker(Kirigami.Theme.disabledTextColor)
            anchors.fill: parent
            radius: root.padding
        }

        Rectangle {
            anchors {
                fill: parent
                leftMargin: 1
                rightMargin: ((1-root.progress) * parent.width) + 1
                topMargin: 1
                bottomMargin: 1
            }
            color: Kirigami.Theme.highlightColor
            radius: root.padding-2
        }
    }

    contentItem: Label {
        id: theLabel
        horizontalAlignment: Text.AlignHCenter
        color: root.inProgress ? Kirigami.Theme.highlightedTextColor : Kirigami.Theme.textColor
    }

    ToolTip.visible: hovered
    ToolTip.text: theLabel.text
}
