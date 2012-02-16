import QtQuick 1.1
import org.kde.plasma.components 0.1

Page
{
    Button {
        id: commitButton
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        text: i18n("Update!")
    }
    
    Item {
        anchors {
            top: commitButton.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        
        ApplicationsList {
            id: apps
            anchors.fill: parent
            stateFilter: (1<<9)//Upgradeable
            visible: apps.count>0
        }
        
        Label {
            anchors.fill: parent
            font.pointSize: 25
            text: i18n("Nothing to update")
            visible: apps.count==0
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}