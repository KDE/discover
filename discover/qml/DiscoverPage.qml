/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.5
import org.kde.kirigami 2.14 as Kirigami

Kirigami.ScrollablePage
{
    id: root

//     Kirigami.Theme.colorSet: Kirigami.Theme.View
    //Kirigami.Theme.inherit: false

    readonly property var s1: Shortcut {
        sequence: StandardKey.MoveToNextPage
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY = Math.min(root.flickable.contentHeight - root.flickable.height,
                                               root.flickable.contentY + root.flickable.height);
        }
    }

    readonly property var s2: Shortcut {
        sequence: StandardKey.MoveToPreviousPage
        enabled: root.isCurrentPage
        onActivated: {
            root.flickable.contentY = Math.max(0, root.flickable.contentY - root.flickable.height);
        }
    }

    readonly property var sClose: Shortcut {
        sequence: StandardKey.Cancel
        enabled: root.isCurrentPage && applicationWindow().pageStack.depth>1
        onActivated: {
            applicationWindow().pageStack.pop()
        }
    }

    readonly property var sRefresh: Shortcut {
        sequence: StandardKey.Refresh
        enabled: root.isCurrentPage && root.supportsRefreshing
        onActivated: {
            if (root.supportsRefreshing)
                root.refreshing = true
        }
    }

    readonly property var readableCharacters: /\w+/
    Keys.onPressed: {
        if(event.text.length > 0 && event.modifiers === Qt.NoModifier && event.text.match(readableCharacters)) {
            window.globalDrawer.suggestSearchText(event.text)
        }
    }
}
