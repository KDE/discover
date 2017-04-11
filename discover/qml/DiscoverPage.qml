/*
 *   Copyright (C) 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
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

import QtQuick 2.5
import org.kde.kirigami 2.0 as Kirigami

Kirigami.ScrollablePage
{
    id: root

    property alias pageOverlay: overlayLoader.sourceComponent

    readonly property Item overlay: Item {
        parent: root
        anchors.fill: parent

        z: 500

        Loader {
            id: overlayLoader
            anchors.fill: parent
        }
    }

    readonly property var s1: Shortcut {
        sequence: StandardKey.MoveToNextPage
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY += root.flickable.height
            root.flickable.returnToBounds()
        }
    }

    readonly property var s2: Shortcut {
        sequence: StandardKey.MoveToPreviousPage
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY -= root.flickable.height
            root.flickable.returnToBounds()
        }
    }

    readonly property var sClose: Shortcut {
        sequence: StandardKey.Cancel
        enabled: root.isCurrentPage && applicationWindow().pageStack.depth>1
        onActivated: {
            applicationWindow().pageStack.goBack()
        }
    }
}
