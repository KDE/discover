/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

Loader {
    id: root

    property QtObject inlineMessage

    active: inlineMessage !== null

    sourceComponent: Kirigami.InlineMessage {
        text: root.inlineMessage.message
        type: root.inlineMessage.type
        icon.name: root.inlineMessage.iconName
        leftPadding: active ? Kirigami.Units.smallSpacing : 0
        rightPadding: active ? Kirigami.Units.smallSpacing : 0
        topPadding: active ? Kirigami.Units.smallSpacing : 0

        Component {
            id: comp
            ConvertDiscoverAction {}
        }
        actions: root.inlineMessage.actions.map(action => comp.createObject(this, { action }))
        visible: true
    }
}
