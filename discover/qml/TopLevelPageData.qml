/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import org.kde.kirigami 2.0

Action {
    property string component
    checked: window.currentTopLevel==component

    onTriggered: {
        if(window.currentTopLevel!=component)
            window.currentTopLevel=component
    }
}
