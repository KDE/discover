/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQml
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

/*
 * Converts a DiscoverAction into a Kirigami.Action so we can use DiscoverActions
 * with QQC2 components
 */
Kirigami.Action {
    required property Discover.DiscoverAction action

    icon.name: action.iconName
    text: action.text
    tooltip: action.toolTip
    visible: action.visible
    onTriggered: action.trigger()
}
