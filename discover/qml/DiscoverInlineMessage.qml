/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Controls as QQC2
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components
import org.kde.discover as Discover

Loader {
    id: root

    property Discover.InlineMessage inlineMessage
    property int position: QQC2.ToolBar.Header

    active: inlineMessage !== null

    sourceComponent: Components.Banner {
        text: root.inlineMessage.message
        type: root.inlineMessage.type
        iconName: root.inlineMessage.iconName
        position: root.position

        Component {
            id: component
            ConvertDiscoverAction {}
        }
        actions: root.inlineMessage.actions.map(action => component.createObject(this, { action }))
        visible: true
    }
}
