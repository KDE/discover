/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts

Loader {
    property Component componentTrue
    property Component componentFalse
    property bool condition

    Layout.minimumHeight: item ? item.Layout.minimumHeight : 0
    Layout.minimumWidth: item ? item.Layout.minimumWidth : 0
    sourceComponent: condition ? componentTrue : componentFalse
}
