/***************************************************************************
 *   Copyright © 2012 Aleix Pol Gonzalez <aleixpol@blue-systems.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import QtQuick.Window 2.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import "navigation.js" as Navigation

Rectangle
{
    id: window
    readonly property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    readonly property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    readonly property Component categoryComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    readonly property Component reviewsComp: Qt.createComponent("qrc:/qml/ReviewsPage.qml")

    //toplevels
    readonly property Component topBrowsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    readonly property Component topInstalledComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    readonly property Component topUpdateComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    readonly property Component topSourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    property Component currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true

    objectName: "DiscoverMainWindow"

    visible: true
    implicitHeight: 800
    implicitWidth: 900

    SystemPalette { id: palette }
    color: palette.base

    function clearSearch() {
        if (toolbar.search)
            toolbar.search.text=""
    }

    Component {
        id: loadingComponent
        Item {
            Label {
                text: i18n("Loading...")
                font.pointSize: 52
                anchors.centerIn: parent
            }
        }
    }

    onCurrentTopLevelChanged: {
        if(currentTopLevel==null)
            return
        window.clearSearch()
        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        while(stackView.depth>1) {
            var obj = stackView.pop()
            if(obj)
                obj.destroy(2000)
        }
        if(stackView.currentItem) {
            stackView.currentItem.destroy(100)
        }
        var page;
        try {
            page = currentTopLevel.createObject(stackView)
//             console.log("created ", currentTopLevel)
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+currentTopLevel.errorString())
        }
        stackView.replace(page, {}, window.status!=Component.Ready)
    }

    property list<Action> awesome: [
        TopLevelPageData {
            iconName: "tools-wizard"
            text: i18n("Discover")
            component: topBrowsingComp
            objectName: "discover"
            shortcut: "Alt+D"
        },
        TopLevelPageData {
            iconName: "applications-other"
            text: TransactionModel.count == 0 ? i18n("Installed") : i18n("Installing...")
            component: topInstalledComp
            objectName: "installed"
            shortcut: "Alt+I"
        },
        TopLevelPageData {
            iconName: "system-software-update"
            text: ResourcesModel.updatesCount==0 ? i18n("No Updates") : i18n("Update (%1)", ResourcesModel.updatesCount)
            enabled: ResourcesModel.updatesCount>0
            component: topUpdateComp
            objectName: "update"
            shortcut: "Alt+U"
        }
    ]

    Connections {
        target: app
        onOpenApplicationInternal: Navigation.openApplication(app)
        onListMimeInternal: Navigation.openApplicationMime(mime)
        onListCategoryInternal: Navigation.openCategoryByName(name)
        onPreventedClose: closePreventedInfo.enabled = true
    }

    Rectangle {
        gradient: Gradient {
            GradientStop { position: 0.0; color: "darkGray" }
            GradientStop { position: 1.0; color: "transparent" }
        }
        height: parent.height/5
        anchors {
            topMargin: toolbar.height
            top: parent.top
            left: parent.left
            right: parent.right
        }
        visible: !fu.visible
    }

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        MuonToolbar {
            Layout.fillWidth: true
            id: toolbar
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: rep.count>0 ? msgColumn.height : 0

            ColumnLayout {
                width: parent.width
                id: msgColumn

                Repeater {
                    id: rep
                    model: MessageActionsModel {
                        filterPriority: QAction.HighPriority
                    }
                    delegate: MessageAction {
                        width: msgColumn.width
                        Layout.fillWidth: true
                        theAction: action
                    }
                }
            }
        }

        MessageAction {
            width: msgColumn.width
            Layout.fillWidth: true
            theAction: Action {
                id: closePreventedInfo
                enabled: false
                text: i18n("Got it");
                tooltip: i18n("Could not close the application, there are tasks that need to be done.")
                onTriggered: {
                    enabled=false
                }
            }
        }

        Breadcrumbs {
            id: fu
            Layout.fillWidth: true
            visible: count>1

            pageStack: stackView
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true

            onDepthChanged: {
                window.clearSearch()
            }
        }
    }
}
