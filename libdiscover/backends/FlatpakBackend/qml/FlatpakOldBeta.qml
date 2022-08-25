/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.15
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.10 as Kirigami
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Kirigami.InlineMessage
{
    id: oldBetaItem
    // resource is set by the creator of the element in ApplicationPage.
    //required property AbstractResource resource
    Layout.fillWidth: true
    text: betaOlderThanStable ? i18nc("@label %1 is the name of an application", "This development version of %1 is outdated. Using the stable version is highly recommended.", resource.name) : i18nc("@label %1 is the name of an application", "A more stable version of %1 is available.", resource.name)
    height: visible ? implicitHeight : 0
    visible: actionsArray.filter(action => action.visible).length > 0
    type: betaOlderThanStable ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information

    property bool betaOlderThanStable: false

    Instantiator {
        id: inst
        model: ResourcesProxyModel {
            allBackends: true
            backendFilter: resource.backend
            resourcesUrl: resource.url
        }
        delegate: Kirigami.Action {
            visible: model.application !== resource && model.application.branch !== "beta" && model.application.branch !== "master"
            text: i18nc("@action: button %1 is the name of a Flatpak repo", "View Stable Version on %1", displayOrigin)
            onTriggered: {
                applicationWindow().pageStack.pop();
                Navigation.openApplication(model.application)
            }
            Component.onCompleted: {
                if (visible) {
                    betaOlderThanStable |= resource.isOlderThan(model.application)
                }
            }
        }

        onObjectAdded: {
            oldBetaItem.actionsArray.splice(index, 0, object)
            oldBetaItem.actions = oldBetaItem.actionsArray = oldBetaItem.actionsArray
        }
        onObjectRemoved: {
            oldBetaItem.actionsArray.splice(index, 1)
            oldBetaItem.actions = oldBetaItem.actionsArray = oldBetaItem.actionsArray
        }
    }

    property var actionsArray: []
    actions: actionsArray
}