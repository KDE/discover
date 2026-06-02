/*
 *   SPDX-FileCopyrightText: 2026 Oliver Beard <olib141@outlook.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kirigami as Kirigami

import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

RowLayout {
    id: root

    property alias application: listener.resource

    Discover.TransactionListener {
        id: listener
    }

    QQC2.Button {
        id: button
        text: "Loading…"
    }

    QQC2.ProgressBar {
        Layout.preferredWidth: button.width
        Layout.preferredHeight: button.height
        //width: button.width
        //height: button.height
        padding: 0
        topPadding: 0
        bottomPadding: 0
        indeterminate: true

        Text {
            anchors.centerIn: parent
            text: button.text
        }
    }
}
