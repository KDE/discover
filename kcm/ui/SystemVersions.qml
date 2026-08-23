/*
 *   SPDX-FileCopyrightText: 2026 Hadi Chokr <hadichokr@icloud.com>
 *
 *   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.kirigami as Kirigami
import org.kde.kirigami.delegates as KD

ColumnLayout {
    id: root

    required property var imageVersions

    readonly property bool busy: root.imageVersions?.busy ?? false

    spacing: 0

    Kirigami.InlineViewHeader {
        Layout.fillWidth: true
        text: i18nc("@title:group", "Keep these system versions")
    }

    QQC2.Label {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.smallSpacing
        Layout.bottomMargin: Kirigami.Units.smallSpacing

        wrapMode: Text.Wrap
        textFormat: Text.PlainText
        text: i18nc("@info", "Versions that are kept stay available to start from the boot menu. Every other version is deleted to make room once newer ones are installed.")
    }

    Repeater {
        id: versions

        model: root.imageVersions

        delegate: KD.CheckSubtitleDelegate {
            id: delegate

            required property int index
            required property string version
            required property string date
            required property bool pinned
            required property bool enforced
            required property bool running

            Layout.fillWidth: true

            enabled: !delegate.enforced && !root.busy
            checked: delegate.pinned || delegate.enforced

            text: delegate.date.length > 0 ? delegate.date : delegate.version
            subtitle: delegate.running
                ? i18nc("@info %1 is a version string", "%1 · Currently running", delegate.version)
                : delegate.version

            onToggled: {
                root.imageVersions.setPinned(delegate.index, checked);
                checked = Qt.binding(() => delegate.pinned || delegate.enforced);
            }
        }
    }

    Kirigami.LoadingPlaceholder {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit
        Layout.bottomMargin: Kirigami.Units.gridUnit

        visible: versions.count === 0 && root.busy
        text: i18nc("@info:placeholder", "Looking for installed versions…")
    }

    Kirigami.PlaceholderMessage {
        Layout.fillWidth: true
        Layout.topMargin: Kirigami.Units.gridUnit
        Layout.bottomMargin: Kirigami.Units.gridUnit

        visible: versions.count === 0 && !root.busy
        icon.name: "system-software-update"
        text: i18nc("@info:placeholder", "No system versions are installed")
    }
}
