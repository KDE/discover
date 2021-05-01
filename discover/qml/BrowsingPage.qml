/*
 *   SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   SPDX-License-Identifier: LGPL-2.0-or-later
 */

import QtQuick 2.4
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation
import org.kde.kirigami 2.14 as Kirigami

DiscoverPage
{
    id: page
    title: i18n("Featured")
    leftPadding: 0
    rightPadding: 0
    topPadding: 0
    bottomPadding: 0

    actions.main: searchAction

    readonly property bool isHome: true

    function searchFor(text) {
        if (text.length === 0)
            return;
        Navigation.openCategory(null, "")
    }

    ColumnLayout {
        anchors.centerIn: parent
        opacity: 0.5

        visible: apps.count === 0 && apps.model.isFetching

        Kirigami.Heading {
            level: 2
            Layout.alignment: Qt.AlignCenter
            text: i18n("Loading...")
        }
        BusyIndicator {
            id: indicator
            Layout.alignment: Qt.AlignCenter
            Layout.preferredWidth: Kirigami.Units.gridUnit * 4
            Layout.preferredHeight: Kirigami.Units.gridUnit * 4
        }
    }

    Kirigami.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - (Kirigami.Units.largeSpacing * 4)

        visible: apps.count === 0 && !apps.model.isFetching

        icon.name: "network-disconnect"
        text: i18n("Unable to load applications")
        explanation: ("Please verify Internet connectivity")
    }

    signal clearSearch()

    readonly property bool compact: page.width < 550 || !applicationWindow().wideScreen


    header: ToolBar {
        width: page.width
        height: visible ? youGotUpdatesHeaderMessage.height + Kirigami.Units.smallSpacing * 4 : 0
        visible: page.compact && ResourcesModel.updatesCount>0
        Kirigami.InlineMessage {
            id: youGotUpdatesHeaderMessage
            visible: true
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                leftMargin: Kirigami.Units.smallSpacing
                rightMargin: Kirigami.Units.smallSpacing
            }
            icon.name: updateAction.icon.name
            text: i18n("Updates are available")
            actions: [
                Kirigami.Action {
                    icon.name: "go-next"
                    text: i18nc("Short for 'show updates'", "Show")
                    onTriggered: { updateAction.trigger(); }
                }
            ]
        }
    }
    Kirigami.CardsListView {
        id: apps
        model: FeaturedModel {}
        activeFocusOnTab: true
        Component.onCompleted: apps.bottomMargin = Kirigami.Units.largeSpacing * 2
        onActiveFocusChanged: if (activeFocus && currentIndex === -1) {
            currentIndex = 0;
        }
        currentIndex: -1
        delegate: ApplicationDelegate {
            application: model.application
            compact: page.compact
        }
    }
}
