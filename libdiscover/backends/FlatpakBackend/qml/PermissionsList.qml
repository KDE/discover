/*
 *   SPDX-FileCopyrightText: 2022 Suhaas Joshi <joshiesuhaas0@gmail.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kcmutils as KCMUtils
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

ColumnLayout {
    id: root

    visible: list.model.rowCount() > 0
    spacing: 0

    FormCard.FormHeader {
        title: i18ndc("libdiscover", "%1 is the name of the application", "Permissions for %1", resource.name)
    }

    FormCard.FormCard {
        Repeater {
            id: list
            model: resource.permissionsModel()

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
                    hoverEnabled: resource.isInstalled
                    down: resource.isInstalled ? undefined : false

                    background {
                        visible: resource.isInstalled
                    }

                    onClicked: {
                        if (resource.isInstalled) {
                            // TODO: Not only open KCM on the app's page, but also focus on relevant permission row
                            KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [resource.ref]);
                        }
                    }
                }
            }
        }

        FormCard.FormDelegateSeparator {}

        FormCard.FormButtonDelegate {
            visible: resource.isInstalled
            text: i18nd("libdiscover", "Configure permissions…")
            icon.name: "configure"
            onClicked: {
                KCMUtils.KCMLauncher.openSystemSettings("kcm_flatpak", [resource.ref]);
            }
        }
    }
}
