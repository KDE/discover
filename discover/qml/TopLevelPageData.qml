/*
 *   SPDX-FileCopyrightText: 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import org.kde.kirigami 2 as Kirigami

Kirigami.Action {
    property string component

    checked: window.currentTopLevel === component

    onToggled: {
        // Since there is no achitecture in place to act as a controller for
        // any of this navigation, exclusive top-level actions have to ensure
        // they can not be unchecked by clicking a second time.
        if (!checked) {
            checked = Qt.binding(() => window.currentTopLevel === component)
        }
    }

    onTriggered: {
        if (window.currentTopLevel !== component) {
            window.currentTopLevel = component
        }
    }
}
