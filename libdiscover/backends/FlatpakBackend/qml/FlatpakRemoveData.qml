/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.kirigami 2.10 as Kirigami

Kirigami.InlineMessage
{
    // resource is set by the creator of the element in ApplicationPage.
    //required property AbstractResource resource
    Layout.fillWidth: true
    text: i18nd("libdiscover", "%1 is not installed but it still has data present.", resource.name)
    visible: resource.hasData && query.count === 0
    height: visible ? implicitHeight : 0

    ResourcesProxyModel {
        id: query
        backendFilter: resource.backend
        resourcesUrl: resource.url
        stateFilter: AbstractResource.Installed
    }

    actions: [
        Kirigami.Action {
            icon.name: "delete"
            text: i18nd("libdiscover", "Delete settings and user data")
            onTriggered: {
                resource.clearUserData()
            }
        }
    ]
}
