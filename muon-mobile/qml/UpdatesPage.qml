import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Page
{
    id: page
    function start() { updates.upgradeAll() }
    ApplicationUpdates {
        id: updates
        onProgress: { progress.value=percentage; message.text+=text+'\n' }
        onDownloadMessage: { message.text+=msg+'\n' }
        onInstallMessage: { message.text+=msg+'\n' }
        
        onUpdatesFinnished: pageStack.pop()
    }
    ProgressBar {
        id: progress
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
        
        Label {
            id: message
            width: parent.width-messageScroll.width
            wrapMode: Text.WordWrap
            onTextChanged: messageFlickable.contentY=message.height
        }
    }
}