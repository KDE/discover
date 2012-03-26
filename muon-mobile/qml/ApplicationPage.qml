import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    id: page
    property QtObject application
    anchors.margins: 5
    
    TabBar {
        id: tabs
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        TabButton {
            tab: applicationOverview
            text: i18n("Overview")
        }
        TabButton {
            tab: holaLabel
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
        }
        
        Label { id: holaLabel; text: "hola" }
        ReviewsView {
            id: reviewsView
            application: page.application
        }
    }
}