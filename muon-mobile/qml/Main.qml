import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation

Item {
    id: window
    width: 800
    property Component categoryComp: Qt.createComponent("qrc:/qml/CategoryPage.qml")
    property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    property Component updatesComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    
    //sebas's hack :D
    Rectangle {
        anchors.fill: parent
        color: theme.backgroundColor
        opacity: .2
        
        Rectangle {
            anchors.fill: parent
            color: theme.textColor
        }
    }
    
    ToolBar {
        id:toolbar
        z: 10
        height: 50
        width: parent.width
        anchors.top: parent.top
        
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
            
            MuonToolButton {
                id: progressButton
                icon: "view-refresh"
                height: parent.height
                checkable: true
                checked: progressBox.visible
                onClicked: progressBox.visible=!progressBox.visible
                visible: progressBox.active

                ProgressView {
                    id: progressBox
                    visible: false
                    anchors.horizontalCenter: progressButton.horizontalCenter
                    anchors.top: progressButton.bottom
                }
            }
            
            MuonToolButton {
                id: usersButton
                icon: "system-users"
                visible: window.state=="loaded"
                height: parent.height
                checkable: true
                checked: connectionBox.visible
                onClicked: connectionBox.visible=!connectionBox.visible
                
                RatingAndReviewsConnection {
                    id: connectionBox
                    visible: false
                    anchors.horizontalCenter: usersButton.horizontalCenter
                    anchors.top: usersButton.bottom
                }
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
        if(state=="loaded") {
            breadcrumbs.pushItem("go-home", i18n("Get Software"), true)
            connectionBox.init()
        }
    }
    
    CategoryPage {
        id: mainPage
    }
    
    ToolBar {
        id: breadcrumbsBar
        anchors {
            top: toolbar.bottom
            left: parent.left
            right: pageToolBar.left
            rightMargin: 10
        }
        height: 30
        z: 0
        
        Breadcrumbs {
            id: breadcrumbs
            anchors.fill: parent
            onClicked: {
                var pos = idx;
                while(pos--) {
                    pageStack.pop(pos>1)
                    breadcrumbs.popItem()
                }
            }
        }
    }
    
    ToolBar {
        id: pageToolBar
        anchors {
            leftMargin: 10
            rightMargin: 10
            top: toolbar.bottom
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
    
    PageStack
    {
        id: pageStack
        anchors {
            bottom: parent.bottom
            top: breadcrumbsBar.bottom
            left: parent.left
            right: parent.right
            margins: 10
        }
        initialPage: window.state=="loaded" ? mainPage : null
        clip: true
        
        toolBar: pageToolBar
    }
}