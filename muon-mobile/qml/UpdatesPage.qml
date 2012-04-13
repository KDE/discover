import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Page
{
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
        id: messageScroll
        orientation: Qt.Vertical
        flickableItem: messageFlickable
        visible: messageFlickable.visible
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
        clip: true
        contentHeight: message.height
        visible: page.state=="updating"
        
        Label {
            id: message
            width: parent.width-messageScroll.width
            wrapMode: Text.WordWrap
            onTextChanged: messageFlickable.contentY=message.height
        }
    }
    
    ApplicationsList {
        id: apps
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            bottom: librariesUpdatesLabel.top
        }
        model: ApplicationProxyModel {
            sortOrder: Qt.AscendingOrder
            stateFilter: (1<<9)//Upgradeable
            stringSortRole: "origin"
            
            Component.onCompleted: sortModel()
        }
        section.property: "origin"
        section.delegate: Label { text: i18n("From %1", section) }
        visible: apps.count>0 && page.state!="updating"
        preferUpgrade: true
        clip: true
    }
    Label {
        id: librariesUpdatesLabel
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            bottomMargin: apps.count>0 ? 0 : page.height/2
        }
        height: app.appBackend.updatesCount==0 ? 0 : 40
        font.pixelSize: height*0.8
        text: i18n("%1 system updates", app.appBackend.updatesCount)
        visible: app.appBackend.updatesCount>0 && page.state!="updating"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    tools: Row {
        anchors.fill: parent
        ToolButton {
            id: commitButton
            text: i18n("Upgrade All!")
            iconSource: "system-software-update"
            visible: app.appBackend.updatesCount>0 && page.state!="updating"
            
            onClicked: {
                updates.upgradeAll();
                page.state = "updating"
            }
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