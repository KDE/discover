/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQml 2.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.AboutPage
{
    contextualActions: [
        Kirigami.Action {
            function removeAmpersand(text) {
                return text.replace("&", "");
            }

            readonly property QtObject action: app.action("help_report_bug")
            text: removeAmpersand(action.text)
            enabled: action.enabled
            onTriggered: action.trigger()
            icon.name: app.iconName(action.icon)
            displayHint: Kirigami.DisplayHint.AlwaysHide
        }
    ]

    aboutData: discoverAboutData
}
