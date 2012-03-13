import QtQuick 1.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation

Item {
    function init() {
        breadcrumbsItem.pushItem("go-home")
    }
    
    ToolBar {
        id: breadcrumbsBar
        anchors {
            top: parent.top
            left: parent.left
            right: pageToolBar.left
            rightMargin: 10
        }
        height: 30
        z: 0
        
        Breadcrumbs {
            id: breadcrumbsItem
            anchors.fill: parent
            onClicked: Navigation.jumpToIndex(pageStack, breadcrumbsItem, idx)
        }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            leftMargin: 10
            rightMargin: 10
            top: parent.top
            right: parent.right
        }
        width: visible ? parent.width/4 : 0
        visible: tools!=null
        
        Behavior on width {
            PropertyAnimation { 
                id: heightAnimation
                duration: 250
            }
        }
    }
    
    PageStack {
        id: pageStack
        anchors {
            top: breadcrumbsBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        property Item breadcrumbs: breadcrumbsItem
        
        initialPage: ApplicationsListPage {
            stateFilter: (1<<8)
            sortRole: 32
            sortOrder: Qt.Ascending
        }
        
        toolBar: pageToolBar
    }
}