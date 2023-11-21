/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

Loader {
    id: root

    property Discover.InlineMessage inlineMessage

    active: inlineMessage !== null

    sourceComponent: Kirigami.InlineMessage {
        text: root.inlineMessage.message
        type: root.inlineMessage.type
        icon.name: root.inlineMessage.iconName

        Component {
            id: component
            ConvertDiscoverAction {}
        }
        actions: root.inlineMessage.actions.map(action => component.createObject(this, { action }))
        visible: true
    }
}
