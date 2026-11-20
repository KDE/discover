// SPDX-FileCopyrightText: 2026 Devin Lin <devin@kde.org>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.bigscreen.ui as BigscreenUI
import org.kde.discover as Discover

BigscreenUI.Page {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: Kirigami.Units.gridUnit

        BigscreenUI.TextField {
            id: searchField

            Layout.topMargin: root.contentMargins.top
            Layout.leftMargin: root.contentMargins.left
            Layout.rightMargin: root.contentMargins.right
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 40

            focus: true
            placeholderText: i18n("Search for apps…")
            inputMethodHints: Qt.ImhNoPredictiveText

            onTextChanged: searchDelay.restart()
            onAccepted: {
                searchDelay.stop();
                appsModel.search = text;
                if (grid.count > 0) {
                    grid.forceActiveFocus();
                }
            }

            Timer {
                id: searchDelay
                interval: 400
                onTriggered: appsModel.search = searchField.text
            }

            KeyNavigation.down: grid
        }

        AppGrid {
            id: grid

            Layout.fillWidth: true
            Layout.fillHeight: true

            busy: appsModel.busy
            placeholderIconName: appsModel.search.length > 0 ? "edit-none" : "search"
            placeholderText: appsModel.search.length > 0
                ? i18n("No apps found for “%1”", appsModel.search)
                : i18n("Search apps")

            model: Discover.ResourcesProxyModel {
                id: appsModel
                sortRole: Discover.ResourcesProxyModel.SearchRelevanceRole
                sortOrder: Qt.DescendingOrder
            }

            KeyNavigation.priority: KeyNavigation.AfterItem
            KeyNavigation.up: searchField
        }
    }
}
