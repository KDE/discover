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
import org.kde.muon 1.0
import org.kde.muon.discover 1.0
import "navigation.js" as Navigation

Rectangle
{
    id: window
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component reviewsComp: Qt.createComponent("qrc:/qml/ReviewsPage.qml")

    //toplevels
    property Component topBrowsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    property Component topInstalledComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    property Component topUpdateComp: Qt.createComponent("qrc:/qml/PresentUpdatesPage.qml")
    property Component topSourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    property Component currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true

    visible: true

    SystemPalette { id: palette }
    color: palette.base

    function clearSearch() { toolbar.search.text="" }
    Timer {
        id: searchTimer
        running: false
        repeat: false
        interval: 200
        onTriggered: { stackView.currentItem.searchFor(toolbar.search.text) }
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

    property list<DiscoverAction> awesome: [
        TopLevelPageData {
            iconName: "tools-wizard"
            text: i18n("Discover")
            component: topBrowsingComp
            objectName: "discover"
            shortcut: "Alt+D"
        },
        TopLevelPageData {
            iconName: "applications-other"
            text: i18n("Installed")
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
        },
        TopLevelPageData {
            iconName: "repository"
            text: i18n("Sources")
            component: topSourcesComp
            objectName: "sources"
            shortcut: "Alt+S"
            enabled: SourcesModel.count>0
        }
    ]

    Connections {
        target: app
        onOpenApplicationInternal: Navigation.openApplication(app)
        onListMimeInternal: Navigation.openApplicationMime(mime)
        onListCategoryInternal: Navigation.openCategoryByName(name)
    }

    ColumnLayout {
        spacing: 0
        anchors.fill: parent

        MuonToolbar {
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

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: (breadcrumbsItem.visible || pageToolBar.visible) ? toolbar.Layout.preferredHeight : 0


            Breadcrumbs {
                id: breadcrumbsItem

                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                    right: pageToolBar.left
                    rightMargin: pageToolBar.visible ? 10 : 0
                }

                pageStack: stackView
            }

            ToolBar {
                id: pageToolBar

                anchors {
                    top: parent.top
                    right: parent.right
                    bottom: parent.bottom
                }
                Layout.minimumHeight: toolbarLoader.item ? toolbarLoader.item.Layout.minimumHeight : 0
                width: toolbarLoader.item ? toolbarLoader.item.width+5 : 0
                visible: width>0

                Loader {
                    id: toolbarLoader
                    sourceComponent: stackView.currentItem ? stackView.currentItem.tools : null
                }

                Behavior on width { NumberAnimation { duration: 250 } }
            }
        }

        StackView {
            id: stackView
            Layout.fillWidth: true
            Layout.fillHeight: true

            onDepthChanged: {
                window.clearSearch()
            }
        }

        ProgressView {
            id: progressBox //used from UpdateProgressPage.qml
            Layout.fillWidth: true
        }
    }
}
