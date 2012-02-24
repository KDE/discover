import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Page
{
    id: page
    state: "show"
    ApplicationUpdates {
        id: updates
        onProgress: { progress.value=percentage; message.text=text }
        onDownloadMessage: { message.text=msg }
        onInstallMessage: { message.text=msg }
    }
    Column {
        spacing: 10
        visible: page.state=="updating"
        anchors.fill: parent
        ProgressBar {
            id: progress
            anchors.right: parent.right
            anchors.left: parent.left
            minimumValue: 0
            maximumValue: 100
        }
        Label {
            id: message
            anchors.right: parent.right
            anchors.left: parent.left
        }
    }
    
    ApplicationsList {
        id: apps
        anchors {
            top: commitButton.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        stateFilter: (1<<9)//Upgradeable
        visible: apps.count>0 && page.state!="updating"
    }
    Button {
        id: commitButton
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        text: i18n("Update!")
        visible: apps.count>0 && page.state!="updating"
        
        onClicked: {
            //TODO: Untested, I don't have anything to update now
            var toupgrade = new Array;
            for(var i=0; i<apps.count; i++) {
                toupgrade.push(apps.model.applicationAt(i))
            }
            updates.updateApplications(toupgrade);
            page.state = "updating"
        }
    }
    
    Label {
        anchors.fill: parent
        font.pointSize: 25
        text: i18n("Nothing to update")
        visible: apps.count==0 && page.state!="updating"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    states: [ State { name: "show" }, State { name: "updating" } ]
}