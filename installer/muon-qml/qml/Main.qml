import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Rectangle {
    id: window
    width: 800
    color: "lightgrey"
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    property bool opening: false
    
    ToolBar {
        id:toolbar
        z: 10
        height: 50
        width: parent.width
        anchors.top: parent.top
        clip: true
        
        Row {
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
            }
            MuonToolButton {
                height: parent.height
                icon: "download"
                text: i18n("Get software")
                onClicked: Navigation.clearOpened()
            }
            MuonToolButton {
                height: parent.height
                icon: "applications-other"
                text: i18n("Installed")
                onClicked: Navigation.openInstalledList()
            }
            MuonToolButton {
                height: parent.height
                icon: "system-software-update"
                text: i18n("Updates")
                onClicked: Navigation.openUpdatePage()
            }
        }
        
        Row {
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
            }
            
            Repeater {
                model: ["software_properties", "quit"]
                
                delegate: MuonToolButton {
                    property QtObject action: app.getAction(modelData)
                    height: parent.height
                    
                    onClicked: action.trigger()
                    enabled: action.enabled
                    icon: action.icon
                }
            }
        }
    }
    
    onStateChanged: { 
        if(state=="loaded")
            breadcrumbs.pushItem("go-home", i18n("Get Software"), true)
    }
    
    CategoryPage {
        id: mainPage
    }
    
    ToolBar {
        id: breadcrumbsBar
        height: 40
        anchors {
            top: toolbar.bottom
            right: parent.right
            left: parent.left
        }
        
        Breadcrumbs {
            id: breadcrumbs
            anchors.fill: parent
            onClicked: {
                var pos = idx;
                while(pos--) { pageStack.pop(); breadcrumbs.popItem() }
            }
            
            onSearchChanged: {
                if(search)
                    pageStack.currentPage.searchFor(search)
            }
        }
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.top: breadcrumbsBar.bottom
        initialPage: window.state=="loaded" ? mainPage : null
        
        toolBar: toolbar
    }
}