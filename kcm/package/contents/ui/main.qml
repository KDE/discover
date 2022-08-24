/*
 *   SPDX-FileCopyrightText: 2020 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 2.3 as QQC2
import org.kde.kirigami 2.14 as Kirigami
import org.kde.kcm 1.3

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

    ColumnLayout {
        spacing: 0

        Kirigami.FormLayout {
            id: unattendedUpdatesLayout
            Layout.fillWidth: true
            QQC2.RadioButton {
                Kirigami.FormData.label: i18n("Update software:")
                text: i18n("Manually")

                QQC2.ButtonGroup.group: autoUpdatesGroup
                checked: !kcm.updatesSettings.useUnattendedUpdates
            }
            QQC2.RadioButton {
                id: automaticallyRadio
                text: i18n("Automatically")

                QQC2.ButtonGroup.group: autoUpdatesGroup
                checked: kcm.updatesSettings.useUnattendedUpdates
            }

            SettingStateBinding {
                configObject: kcm.updatesSettings
                settingName: "useUnattendedUpdates"
                target: automaticallyRadio
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(Kirigami.Units.gridUnit * 25, Math.round(root.width * 0.6))

            wrapMode: Text.WordWrap
            visible: automaticallyRadio.checked
            horizontalAlignment: Text.AlignHCenter
            font: Kirigami.Theme.smallFont
            text: xi18nc("@info", "Software updates will be downloaded automatically when they become available. Updates for applications will be installed immediately, while system updates will be installed the next time the computer is restarted.")
        }

        Kirigami.FormLayout {
            Layout.fillWidth: true
            twinFormLayouts: unattendedUpdatesLayout

            QQC2.ComboBox {
                Kirigami.FormData.label: i18nc("@title:group", "Notification frequency:")
                model: [
                    i18nc("@item:inlistbox", "Daily"),
                    i18nc("@item:inlistbox", "Weekly"),
                    i18nc("@item:inlistbox", "Monthly"),
                    i18nc("@item:inlistbox", "Never")
                ]

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

            QQC2.CheckBox {
                id: offlineUpdatesBox
                Kirigami.FormData.label: i18n("Use offline updates:")
                enabled: !kcm.discoverSettings.isUseOfflineUpdatesImmutable
                checked: kcm.discoverSettings.useOfflineUpdates
                onToggled: {
                    kcm.discoverSettings.useOfflineUpdates = checked
                }
            }

            SettingStateBinding {
                configObject: kcm.discoverSettings
                settingName: "useOfflineUpdates"
                target: offlineUpdatesBox
            }
        }

        QQC2.Label {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(Kirigami.Units.gridUnit * 25, Math.round(root.width * 0.6))

            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            font: Kirigami.Theme.smallFont
            text: i18n("Offline updates maximize system stability by applying changes while restarting the system. Using this update mode is strongly recommended.")
        }
    }
}
