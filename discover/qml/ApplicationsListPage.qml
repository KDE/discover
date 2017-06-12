/*
 *   Copyright (C) 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library/Lesser General Public License
 *   version 2, or (at your option) any later version, as published by the
 *   Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library/Lesser General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 2.5
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import "navigation.js" as Navigation
import org.kde.discover.app 1.0
import org.kde.discover 1.0
import org.kde.kirigami 2.0 as Kirigami

DiscoverPage {
    id: page
    readonly property var model: appsModel
    property alias category: appsModel.filteredCategory
    property alias sortRole: appsModel.sortRole
    property alias sortOrder: appsModel.sortOrder
    property alias originFilter: appsModel.originFilter
    property alias mimeTypeFilter: appsModel.mimeTypeFilter
    property alias stateFilter: appsModel.stateFilter
    property alias extend: appsModel.extends
    property alias search: appsModel.search
    property alias resourcesUrl: appsModel.resourcesUrl
    property alias isBusy: appsModel.isBusy
    property alias count: apps.count
    property alias listHeader: apps.header
    property bool compact: page.width < 500 || Helpers.isCompact

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories
    title: category ? category.name : ""

    onSearchChanged: {
        if (search.length === 0)
            Navigation.openHome()
        appsModel.sortOrder = Qt.AscendingOrder
    }
    signal clearSearch()

    ListView {
        id: apps
        section.delegate: Label {
            text: section
            anchors {
                right: parent.right
            }
        }

        headerPositioning: ListView.OverlayHeader
        header: CategoryDisplay {
            category: appsModel.filteredCategory
            search: appsModel.search
        }
        model: ResourcesProxyModel {
            id: appsModel
            sortRole: ResourcesProxyModel.RatingCountRole
            sortOrder: Qt.DescendingOrder
        }
        spacing: Kirigami.Units.gridUnit
        currentIndex: -1
        delegate: ApplicationDelegate {
            x: Kirigami.Units.gridUnit
            width: ListView.view.width - Kirigami.Units.gridUnit*2
            application: model.application
            compact: page.compact
        }

        Kirigami.Label {
            anchors.centerIn: parent
            opacity: apps.count == 0 && !appsModel.isBusy ? 0.3 : 0
            Behavior on opacity { PropertyAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad; } }
            text: i18n("Sorry, nothing found...")
        }

        BusyIndicator {
            id: busyIndicator
            anchors {
                top: parent.bottom
                horizontalCenter: parent.horizontalCenter
                margins: Kirigami.Units.largeSpacing
            }
            running: false
            opacity: 0
            states: [
                State {
                    name: "running";
                    when: appsModel.isBusy
                    PropertyChanges { target: busyIndicator; opacity: 1; running: true; }
                    AnchorChanges { target: busyIndicator; anchors.bottom: parent.bottom; anchors.top: undefined; }
                }
            ]
            transitions: [
                Transition {
                    from: ""
                    to: "running"
                    SequentialAnimation {
                        PauseAnimation { duration: Kirigami.Units.longDuration * 5; }
                        ParallelAnimation {
                            AnchorAnimation { duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad; }
                            PropertyAnimation { property: "opacity"; duration: Kirigami.Units.longDuration; easing.type: Easing.InOutQuad; }
                        }
                    }
                },
                Transition {
                    from: "running"
                    to: ""
                    ParallelAnimation {
                        AnchorAnimation { duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad; }
                        PropertyAnimation { property: "opacity"; duration: Kirigami.Units.shortDuration; easing.type: Easing.InOutQuad; }
                    }
                }
            ]
            Kirigami.Label {
                id: busyLabel
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    bottom: parent.top
                }
                text: i18n("Still looking...")
                opacity: 0
                states: [
                    State {
                        name: "running";
                        when: busyIndicator.opacity === 1;
                        PropertyChanges { target: busyLabel; opacity: 1; }
                    }
                ]
                transitions: Transition {
                    from: ""
                    to: "running"
                    SequentialAnimation {
                        PauseAnimation { duration: Kirigami.Units.longDuration * 5; }
                        PropertyAnimation { property: "opacity"; duration: Kirigami.Units.longDuration * 10; easing.type: Easing.InOutCubic; }
                    }
                }
            }
        }
    }
}
