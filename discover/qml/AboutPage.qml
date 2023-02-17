/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQml 2.1
import org.kde.kirigami 2.14 as Kirigami

Kirigami.AboutPage {
    readonly property bool isHome: true
    aboutData: discoverAboutData
}
