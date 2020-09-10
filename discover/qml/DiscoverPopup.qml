/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.8
import QtQuick.Controls 2.1
import org.kde.kirigami 2.1 as Kirigami

Popup {
    id: overlay
    parent: applicationWindow().overlay
    bottomPadding: Kirigami.Units.largeSpacing
    topPadding: Kirigami.Units.largeSpacing

    x: (parent.width - width)/2
    y: (parent.height - height)/2
    width: Math.min(parent.width - Kirigami.Units.gridUnit * 3, Kirigami.Units.gridUnit * 50)
    height: Math.min(view.contentHeight + bottomPadding + topPadding, parent.height * 4/5)
}
