/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQml 2.15
import org.kde.kirigami 2.15 as Kirigami

/*
 * Converts a DiscoverAction into a Kirigami.Action so we can use DiscoverActions
 * with QQC2 components
 */
Kirigami.Action {
    property QtObject action
    text: action.text
    tooltip: action.toolTip
    visible: action.visible
    onTriggered: action.trigger()
}
