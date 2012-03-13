import QtQuick 1.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation

Item {
    function init() {
        breadcrumbsItem.pushItem("go-home")
    }
    
    CategoryPage {
        id: mainPage
    }
    
    ToolBar {
        id: breadcrumbsItemBar
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
            NumberAnimation { duration: 250 }
        }
    }
    
    PageStack
    {
        id: pageStack
        anchors {
            margins: 10
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            top: breadcrumbsItemBar.bottom
        }
        property alias breadcrumbs: breadcrumbsItem
        
        initialPage: window.state=="loaded" ? mainPage : null
        clip: true
        
        toolBar: pageToolBar
    }
}