/*
 *   SPDX-FileCopyrightText: 2022 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app
import org.kde.discover.qml
import org.kde.kirigami as Kirigami

Kirigami.InlineMessage {
    id: root

    required property Discover.AbstractResource resource

    property bool __betaOlderThanStable: false

    Layout.fillWidth: true

    text: __betaOlderThanStable
        ? i18ndc("libdiscover", "@label %1 is the name of an application", "This development version of %1 is outdated. Using the stable version is highly recommended.", resource.name)
        : i18ndc("libdiscover", "@label %1 is the name of an application", "A more stable version of %1 is available.", resource.name)

    height: visible ? implicitHeight : 0
    visible: actions.some(action => action?.visible)
    type: __betaOlderThanStable ? Kirigami.MessageType.Warning : Kirigami.MessageType.Information

    onResourceChanged: {
        __betaOlderThanStable = false
        for (const action of actions) {
            action.reset()
        }
    }

    Instantiator {
        id: instantiator
        model: Discover.ResourcesProxyModel {
            allBackends: true
            backendFilter: root.resource.backend
            resourcesUrl: root.resource.url
        }
        active: root.resource.isDesktopApp
        delegate: Kirigami.Action {
            required property var model

            readonly property int versionCompare: root.resource.versionCompare(model.application)

            visible: instantiator.active && model.application !== root.resource && model.application.branch !== "beta" && model.application.branch !== "master" && versionCompare !== 0
            text: i18ndc("libdiscover", "@action: button %1 is the name of a Flatpak repo", "View Stable Version on %1", model.displayOrigin)

            onTriggered: {
                applicationWindow().pageStack.pop();
                Navigation.openApplication(model.application);
            }

            Component.onCompleted: reset()

            function reset() {
                root.__betaOlderThanStable |= versionCompare < 0
            }
        }

        onObjectAdded: (index, object) => {
            root.actions.push(object);
        }
        onObjectRemoved: (index, object) => {
            root.actions = root.actions.filter(action => action !== object)
        }
    }
}
