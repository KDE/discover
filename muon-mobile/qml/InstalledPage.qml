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
        
        tools:  Item {
            anchors.fill: parent
            Breadcrumbs {
                anchors {
                    left: parent.left
                    right: searchField.right
                    top: parent.top
                    bottom: parent.bottom
                }
                id: breadcrumbsItem
                onClicked: Navigation.jumpToIndex(pageStack, breadcrumbsItem, idx)
            }
            
            TextField {
                id: searchField
                anchors {
                    right: parent.right
                    verticalCenter: parent.verticalCenter
                }
                height: parent.height
                visible: pageStack.currentPage!=null && pageStack.currentPage.searchFor!=null
                placeholderText: i18n("Search...")
                onTextChanged: if(text.length>3) pageStack.currentPage.searchFor(text)
//                 onAccepted: pageStack.currentPage.searchFor(text) //TODO: uncomment on kde 4.9
            }
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
        width: visible ? 100 : 0
        visible: tools!=null
        
        Behavior on width {
            PropertyAnimation { 
                duration: 250
            }
        }
    }
    
    PageStack {
        id: pageStack
        clip: true
        anchors {
            top: breadcrumbsBar.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            margins: 10
        }
        property Item breadcrumbs: breadcrumbsItem
        
        initialPage: ApplicationsListPage {
            stateFilter: (1<<8)
            sortRole: "origin"
            sortOrder: 0
            
            section.property: "origin"
            section.delegate: Label { text: i18n("From %1", section) }
        }
        
        toolBar: pageToolBar
    }
}