import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item
{
    anchors.margins: 10
    
    id: page
    state: "show"
    ApplicationUpdates {
        id: updates
        onProgress: { progress.value=percentage; message.text+=text+'\n' }
        onDownloadMessage: { message.text+=msg+'\n' }
        onInstallMessage: { message.text+=msg+'\n' }
        
        onUpdatesFinnished: page.state="show"
    }
    ProgressBar {
        id: progress
        visible: parent.state=="updating"
        anchors {
            right: parent.right
            left: parent.left
            top: parent.top
        }
        
        minimumValue: 0
        maximumValue: 100
    }
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: messageFlickable
        anchors {
            top: messageFlickable.top
            bottom: messageFlickable.bottom
            right: parent.right
        }
    }
    Flickable {
        id: messageFlickable
        anchors {
            top: progress.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
        }
        TextArea {
            id: message
            readOnly: true
            visible: parent.state=="updating"
        }
    }
    
    ApplicationsList {
        id: apps
        anchors {
            top: commitButton.bottom
            left: parent.left
            right: parent.right
            bottom: librariesUpdatesLabel.top
        }
        section.property: "origin"
        section.delegate: Label { text: i18n("From %1", section) }
        stateFilter: (1<<9)//Upgradeable
        sortRole: "origin"
        sortOrder: 0
        visible: apps.count>0 && page.state!="updating"
        preferUpgrade: true
    }
    Label {
        id: librariesUpdatesLabel
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: apps.count>0 ? 0 : page.height/2
        }
        height: app.appBackend.updatesCount==0 ? 0 : 30
        font.pointSize: 25
        text: i18n("%1 system updates", app.appBackend.updatesCount)
        visible: app.appBackend.updatesCount>0 && page.state!="updating"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    Button {
        id: commitButton
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        text: i18n("Upgrade All!")
        visible: app.appBackend.updatesCount>0 && page.state!="updating"
        
        onClicked: {
            updates.upgradeAll();
            page.state = "updating"
        }
    }
    
    Label {
        anchors.fill: parent
        font.pointSize: 25
        text: i18n("Nothing to update")
        visible: app.appBackend.updatesCount==0 && page.state!="updating"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    states: [ State { name: "show" }, State { name: "updating" } ]
}