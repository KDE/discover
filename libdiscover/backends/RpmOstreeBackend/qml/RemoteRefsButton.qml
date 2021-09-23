/*
 *   SPDX-FileCopyrightText: 2021 Mariam Fahmy Sobhy <mariamfahmy66@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.1 as Kirigami

Button
{
    id: root
    text: i18n("Switch to %1 %2", resource.packageName(), resource.getNextMajorVersion())

    onClicked: resource.rebaseToNewVersion()
    visible: resource.isInstalled && resource.isBooted && resource.isNextMajorVersionAvailable()
}

