/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick
import org.kde.kirigamiaddons.formcard as FormCard

FormCard.AboutPage {
    readonly property bool isHome: true
    aboutData: discoverAboutData
}
