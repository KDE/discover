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
import QtQuick.Controls 2.1 as QQC2
import QtQuick.Layouts 1.1
import QtQuick.Window 2.2
import "navigation.js" as Navigation
import org.kde.discover.app 1.0
import org.kde.discover 2.0
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
    property alias extending: appsModel.extending
    property alias search: appsModel.search
    property alias resourcesUrl: appsModel.resourcesUrl
    property alias isBusy: appsModel.isBusy
    property alias allBackends: appsModel.allBackends
    property alias count: apps.count
    property alias listHeader: apps.header
    property alias listHeaderPositioning: apps.headerPositioning
    property bool compact: page.width < 500 || !applicationWindow().wideScreen

    property bool canNavigate: true
    readonly property alias subcategories: appsModel.subcategories
    title: category ? category.name : ""

    signal clearSearch()

    supportsRefreshing: true
    onRefreshingChanged: if (refreshing) {
        appsModel.invalidateFilter()
        refreshing = false
    }

    contextualActions: [
        Kirigami.Action {
            text: i18n("Sort")
            Kirigami.Action {
                text: i18n("Name")
                onTriggered: {
                    appsModel.sortRole = ResourcesProxyModel.NameRole
                    appsModel.sortOrder = Qt.AscendingOrder
                }
                checkable: true
                checked: appsModel.sortRole == ResourcesProxyModel.NameRole
            }
            Kirigami.Action {
                text: i18n("Rating")
                onTriggered: {
                    appsModel.sortRole = ResourcesProxyModel.RatingPointsRole
                    appsModel.sortOrder = Qt.DescendingOrder
                }
                checkable: true
                checked: appsModel.sortRole == ResourcesProxyModel.RatingPointsRole
            }
            Kirigami.Action {
                text: i18n("Size")
                onTriggered: {
                    appsModel.sortRole = ResourcesProxyModel.SizeRole
                    appsModel.sortOrder = Qt.AscendingOrder
                }
                checkable: true
                checked: appsModel.sortRole == ResourcesProxyModel.SizeRole
            }
        }
    ]

    ListView {
        id: apps

        anchors {
            top: parent.top
            topMargin: Kirigami.Units.gridUnit
        }

        section.delegate: QQC2.Label {
            text: section
            anchors {
                right: parent.right
            }
        }

        model: ResourcesProxyModel {
            id: appsModel
            sortRole: ResourcesProxyModel.SortableRatingRole
            sortOrder: search.length>0 ? Qt.AscendingOrder : Qt.DescendingOrder
            onBusyChanged: if (isBusy) {
                apps.currentIndex = -1
            }
        }
        spacing: Kirigami.Units.gridUnit
        currentIndex: -1
        delegate: ApplicationDelegate {
            x: Kirigami.Units.gridUnit
            width: ListView.view.width - Kirigami.Units.gridUnit*2
            application: model.application
            compact: page.compact
        }

        QQC2.Label {
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
            QQC2.Label {
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
