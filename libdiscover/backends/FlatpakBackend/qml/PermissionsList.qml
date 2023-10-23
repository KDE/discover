/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: root

    required property Discover.AbstractResource resource
    Discover.Activatable.active: list.model.rowCount() > 0
    spacing: 0

    FormCard.FormHeader {
        title: i18ndc("libdiscover", "Permission to access system resources and hardware devices", "Permissions")
    }

    FormCard.FormCard {
        Repeater {
            id: list
            model: root.resource.permissionsModel()

            delegate: ColumnLayout {
                id: delegate

                required property int index
                required property string brief
                required property string description
                required property string icon

                FormCard.FormDelegateSeparator {
                    visible: delegate.index !== 0
                }

                FormCard.FormButtonDelegate {
                    text: delegate.brief
                    description: delegate.description
                    icon.name: delegate.icon

                    // so that it gets neither hover nor pressed appearance when it's not interactive
                    hoverEnabled: root.resource.isInstalled
                    down: root.resource.isInstalled ? undefined : false

                    background {
                        visible: root.resource.isInstalled
                    }

                    onClicked: {
                        if (resource.isInstalled) {
                            // TODO: Not only open KCM on the app's page, but also focus on relevant permission row
                            KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [root.resource.ref]);
                        }
                    }
                }
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            visible: root.resource.isInstalled
            text: i18nd("libdiscover", "Configure permissions…")
            icon.name: "configure"
            onClicked: {
                KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [root.resource.ref]);
            }
        }
    }
}
