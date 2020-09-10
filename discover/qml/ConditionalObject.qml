/*
 *   SPDX-FileCopyrightText: 2018 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1

QtObject
{
    id: root

    property Component componentTrue
    property Component componentFalse
    property bool condition

    onConditionChanged: {
        if (object)
            object.destroy(100)

        var component = (condition ? componentTrue : componentFalse)
        object = component ? component.createObject(root) : null
    }

    property QtObject object
}
