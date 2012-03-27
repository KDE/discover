import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    id: page
    property QtObject application
    anchors.margins: 5
    
    Item {
        id: intro
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        height: icon.height
        QIconItem {
            id: icon
            anchors.top: parent.top
            anchors.left: parent.left
            width: 40
            height: 40
            
            icon: application.icon
        }
        
        Column {
            id: header
            anchors.top: parent.top
            anchors.left: icon.right
            anchors.right: parent.right
            anchors.leftMargin: 5
            spacing: 5
            
            Text {
                text: application.name
                width: parent.width
                font.bold: true
            }
            Label {
                text: application.comment
                wrapMode: Text.WordWrap
                width: parent.width
            }
        }
        
        InstallApplicationButton {
            id: installButton
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            application: page.application
        }
    }
    
    TabBar {
        id: tabs
        anchors {
            top: intro.bottom
            left: parent.left
            right: parent.right
            margins: 10
        }
        TabButton {
            tab: applicationOverview
            text: i18n("Overview")
        }
        TabButton {
            tab: addonsView
            text: i18n("Add-ons")
        }
        TabButton {
            tab: reviewsView
            text: i18n("Reviews")
        }
    }
    
    TabGroup {
        id: currentView
        currentTab: tabs.currentTab
        anchors {
            top: tabs.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        
        ApplicationOverview {
            id: applicationOverview
            application: page.application
        }
        
        AddonsView {
            id:addonsView
            application: page.application
        }
        
        ReviewsView {
            id: reviewsView
            application: page.application
        }
    }
}