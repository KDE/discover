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
import org.kde.plasma.components 2.0 as PlasmaComponents
import org.kde.muon.discover 1.0
import "navigation.js" as Navigation

ApplicationWindow {
    id: window
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component reviewsComp: Qt.createComponent("qrc:/qml/ReviewsPage.qml")

    //toplevels
    property Component topBrowsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    property Component topInstalledComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    property Component topSourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    property Component currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true

    width: 500
    height: 500
    visible: true
//     toolBar: pageToolBar

    Binding {
        target: app.searchWidget
        property: "enabled"
        value: pageStack.currentPage!=null && pageStack.currentPage.searchFor!=null
    }
    function clearSearch() { app.searchWidget.text="" }
    Connections {
        target: app.searchWidget
        onTextChanged: searchTimer.running = true
        onEditingFinished: if(app.searchWidget.text == "" && backAction.enabled) {
            backAction.trigger()
        }
    }
    Timer {
        id: searchTimer
        running: false
        repeat: false
        interval: 200
        onTriggered: { pageStack.currentPage.searchFor(app.searchWidget.text) }
    }
    
    Component {
        id: loadingComponent
        Page {
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
        while(pageStack.depth>1) {
            var obj = pageStack.pop()
            if(obj)
                obj.destroy(2000)
        }
        if(pageStack.currentPage) {
            pageStack.currentPage.destroy(100)
        }
        var page;
        try {
            page = currentTopLevel.createObject(pageStack)
//             console.log("created ", currentTopLevel, Navigation.rootPagesCache[currentTopLevel])
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+currentTopLevel.errorString())
        }
        pageStack.replace(page, {}, window.status!=Component.Ready)
    }
    
    DiscoverAction {
        id: backAction
        objectName: "back"
        iconName: "go-previous"
        enabled: window.navigationEnabled && breadcrumbsItem.count>1
        mainWindow: app
        text: i18n("Back")
        priority: "LowPriority"
        shortcut: "Alt+Up"
        onTriggered: {
            breadcrumbsItem.popItem(false)
        }
    }
    TopLevelPageData {
        iconName: "tools-wizard"
        text: i18n("Discover")
        component: topBrowsingComp
        objectName: "discover"
        shortcut: "Alt+D"
    }
    TopLevelPageData {
        iconName: "applications-other"
        text: resourcesModel.updatesCount==0 ? i18n("Installed") : i18np("Installed (%1 update)", "Installed (%1 updates)", resourcesModel.updatesCount)
        component: topInstalledComp
        objectName: "installed"
        shortcut: "Alt+I"
    }
    TopLevelPageData {
        iconName: "repository"
        text: i18n("Sources")
        component: topSourcesComp
        objectName: "sources"
        shortcut: "Alt+S"
    }
    
    Connections {
        target: app
        onOpenApplicationInternal: Navigation.openApplication(app)
        onListMimeInternal: Navigation.openApplicationMime(mime)
        onListCategoryInternal: Navigation.openCategoryByName(name)
    }
    
    ToolBar {
        id: breadcrumbsItemBar
        anchors {
            top: parent.top
            left: parent.left
            right: pageToolBar.left
            rightMargin: pageToolBar.visible ? 10 : 0
        }
        height: breadcrumbsItem.count<=1 ? 0 : 30
        
        tools: Breadcrumbs {
                id: breadcrumbsItem
                anchors.fill: parent
                pageStack: pageStack
                onPoppedPages: window.clearSearch()
                Component.onCompleted: breadcrumbsItem.pushItem("go-home")
            }
        Behavior on height { NumberAnimation { duration: 250 } }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            top: parent.top
            right: parent.right
        }
        height: visible ? 30 : 0
        width: tools!=null ? tools.childrenRect.width+5 : 0
        visible: width>0
        
        Behavior on width { NumberAnimation { duration: 250 } }
    }
    
    PageStack {
        id: pageStack
        toolBar: pageToolBar
        anchors {
            bottom: progressBox.top
            top: parent.top
            left: parent.left
            right: parent.right
            topMargin: Math.max(breadcrumbsItemBar.height, pageToolBar.height)
        }
        onDepthChanged: {
            if(depth==1) {
                breadcrumbsItem.removeAllItems()
            }
        }
    }
    
    ProgressView {
        id: progressBox
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }
}
