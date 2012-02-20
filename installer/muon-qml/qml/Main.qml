import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Rectangle {
    id: window
    width: 800
    height: 600
    color: "lightgrey"
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    property bool opening: false
    
    ToolBar {
        id:toolbar
        z: 10
        height: 40
        width: parent.width
        anchors.top: parent.top
        clip: true
        
        Row {
            id: tools
            spacing: 5
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                topMargin: 5
            }
            
            Repeater {
                model: app.actions
                delegate: ToolButton {
                width: height; height: parent.height
                    
                    onClicked: modelData.trigger()
                    enabled: modelData.enabled
                    
                    QIconItem {
                        anchors.margins: 10
                        anchors.fill: parent
                        icon: modelData.icon
                    }
                }
            }
            
            ToolButton {
                width: height; height: parent.height
                iconSource: "applications-other"
                onClicked: Navigation.openInstalledList()
            }
            
            ToolButton {
                width: height; height: parent.height
                iconSource: "system-software-update"
                onClicked: Navigation.openUpdatePage()
            }
        }
        
        Breadcrumbs {
            id: breadcrumbs
            clip: true
            anchors {
                top: parent.top
                bottom: parent.bottom
                right: parent.right
                left: tools.right
            }
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
    
    onStateChanged: { 
        if(state=="loaded")
            breadcrumbs.pushItem("go-home", i18n("Get Software"), true)
    }
    
    CategoryPage {
        id: mainPage
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.bottom: parent.bottom
        anchors.top: toolbar.bottom
        initialPage: window.state=="loaded" ? mainPage : null
        
        toolBar: toolbar
    }
}