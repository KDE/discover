/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    id: root
    // resource is set by the creator of the element in ApplicationPage.
    Layout.fillWidth: true
    text: i18nd("libdiscover", "%1 is not installed but it still has data present.", resource.name)
    visible: resource.hasData && query.count === 0
    height: visible ? implicitHeight : 0

    Discover.ResourcesProxyModel {
        id: query
        backendFilter: resource.backend
        resourcesUrl: resource.url
        stateFilter: Discover.AbstractResource.Installed
    }

    actions: [
        Kirigami.Action {
            icon.name: "delete"
            text: i18nd("libdiscover", "Delete settings and user data")
            onTriggered: {
                enabled = false
                root.text = i18nd("libdiscover", "Clearing settings and user data…")
                resource.clearUserData()
            }
        }
    ]
}
