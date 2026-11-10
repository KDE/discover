/*
 *   SPDX-FileCopyrightText: 2012-2023 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *   SPDX-FileCopyrightText: 2020-2022 Nate Graham <nate@kde.org>
 *   SPDX-FileCopyrightText: 2023-2024 ivan tkachenko <me@ratijas.tk>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.ki18n
import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

Kirigami.OverlaySheet {
    id: root

    property alias application: addonsModel.application
    property bool isInstalling: false

    readonly property alias addonsCount: listview.count
    readonly property bool containsAddons: listview.count > 0 || isExtended
    readonly property bool isExtended: Discover.ResourcesModel.isExtended(application.appstreamId)

    title: KI18n.i18nc("@title:window page title, addons page, %1 is app name", "Addons for %1", application.name)

    onOpened: {
        listview.forceActiveFocus(Qt.PopupFocusReason);
    }

    ListView {
        id: listview

        implicitWidth: Kirigami.Units.gridUnit * 25

        visible: root.containsAddons
        enabled: !root.isInstalling

        activeFocusOnTab: true
        keyNavigationEnabled: true
        keyNavigationWraps: false

        model: Discover.ApplicationAddonsModel {
            id: addonsModel
        }

        delegate: KD.CheckSubtitleDelegate {
            required property int index
            required property var model

            width: ListView.view.width

            enabled: !root.isInstalling

            icon.width: 0
            text: model.display
            subtitle: model.toolTip

            checked: model.checked
            highlighted: ListView.isCurrentItem

            onToggled: {
                addonsModel.changeState(model.packageName, checked);
                ListView.view.currentIndex = index;
            }
        }
    }

    footer: RowLayout {
        id: footer

        spacing: Kirigami.Units.smallSpacing

        readonly property bool active: addonsModel.hasChanges && !root.isInstalling

        QQC2.Button {
            text: KI18n.i18nc("@action:button show more", "More…")
            visible: root.application.appstreamId.length > 0 && root.isExtended
            onClicked: Navigation.openExtends(root.application.appstreamId, root.application.name)
        }

        Item { Layout.fillWidth: true }

        QQC2.Button {
            icon.name: "dialog-ok"
            text: KI18n.i18nc("@action:button install/remove addons", "Apply Changes")
            onClicked: addonsModel.applyChanges()

            enabled: footer.active
        }

        QQC2.Button {
            icon.name: "document-revert"
            text: KI18n.i18nc("@action:button revert changes, don't install/remove addons", "Reset")
            onClicked: addonsModel.discardChanges()

            enabled: footer.active
        }
    }
}
