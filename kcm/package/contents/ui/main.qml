/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kcm 1.6

SimpleKCM {
    id: root

    ConfigModule.buttons: ConfigModule.Default | ConfigModule.Apply
    QQC2.ButtonGroup {
        id: autoUpdatesGroup
        onCheckedButtonChanged: {
            kcm.updatesSettings.useUnattendedUpdates = automaticallyRadio.checked
        }
    }

    implicitWidth: Kirigami.Units.gridUnit * 38
    implicitHeight: Kirigami.Units.gridUnit * 35


    Kirigami.FormLayout {
        width: parent.width

        QQC2.RadioButton {
            Kirigami.FormData.label: i18n("Update software:")
            text: i18n("Manually")

            QQC2.ButtonGroup.group: autoUpdatesGroup
            checked: !kcm.updatesSettings.useUnattendedUpdates
        }
        RowLayout {
            spacing: Kirigami.Units.smallSpacing

            QQC2.RadioButton {
                id: automaticallyRadio
                text: i18n("Automatically")

                QQC2.ButtonGroup.group: autoUpdatesGroup
                checked: kcm.updatesSettings.useUnattendedUpdates
            }

            ContextualHelpButton {
                toolTipText: xi18nc("@info", "Software updates will be downloaded automatically when they become available. Updates for applications will be installed immediately, while system updates will be installed the next time the computer is restarted.")
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        SettingStateBinding {
            configObject: kcm.updatesSettings
            settingName: "useUnattendedUpdates"
            target: automaticallyRadio
        }

        QQC2.ComboBox {
            Kirigami.FormData.label: kcm.updatesSettings.useUnattendedUpdates ? i18nc("@title:group", "Update frequency:") : i18nc("@title:group", "Notification frequency:")

            readonly property var updatesFrequencyModel: [
                i18nc("@item:inlistbox", "Daily"),
                i18nc("@item:inlistbox", "Weekly"),
                i18nc("@item:inlistbox", "Monthly"),
                i18nc("@item:inlistbox", "Never")
            ]

            // Same as updatesFrequencyModel but without "Never"
            readonly property var unattendedUpdatesFrequencyModel: [
                updatesFrequencyModel[0],
                updatesFrequencyModel[1],
                updatesFrequencyModel[2],
            ]

            model: kcm.updatesSettings.useUnattendedUpdates ? unattendedUpdatesFrequencyModel : updatesFrequencyModel

            readonly property var options: [
                60 * 60 * 24,
                60 * 60 * 24 * 7,
                60 * 60 * 24 * 30,
                -1
            ]

            currentIndex: {
                let index = -1
                for (const i in options) {
                    if (options[i] === kcm.updatesSettings.requiredNotificationInterval) {
                        index = i
                    }
                }
                return index
            }
            onActivated: {
                kcm.updatesSettings.requiredNotificationInterval = options[index]
            }
            SettingStateProxy {
                id: settingState
                configObject: kcm.updatesSettings
                settingName: "requiredNotificationInterval"
            }
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        RowLayout {
            spacing: Kirigami.Units.smallSpacing
            Kirigami.FormData.label: i18n("Use offline updates:")

            Item {
                Kirigami.FormData.isSection: true
            }

            QQC2.CheckBox {
                id: offlineUpdatesBox
                enabled: !kcm.discoverSettings.isUseOfflineUpdatesImmutable
                checked: kcm.discoverSettings.useOfflineUpdates
                onToggled: {
                    kcm.discoverSettings.useOfflineUpdates = checked
                }
            }

            ContextualHelpButton {
                toolTipText: i18n("Offline updates maximize system stability by applying changes while restarting the system. Using this update mode is strongly recommended.")
            }
        }

        SettingStateBinding {
            configObject: kcm.discoverSettings
            settingName: "useOfflineUpdates"
            target: offlineUpdatesBox
        }
    }
}
