import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.muon 1.0

Page
{
    id: page
    property real actualWidth: width-Math.pow(width/70, 2)
    property real sideMargin: (width-actualWidth)/2
    
    function start() { updates.upgradeAll() }
    ApplicationUpdates {
        id: updates
        onProgress: { progress.value=percentage; message.text+=text+'\n' }
        onDownloadMessage: { message.text+=msg+'\n' }
        onInstallMessage: { message.text+=msg+'\n' }
        
        onUpdatesFinnished: pageStack.pop()
    }
    onVisibleChanged: window.navigationEnabled=!visible

    ProgressBar {
        id: progress
        anchors {
            right: parent.right
            left: parent.left
            top: parent.top
            rightMargin: sideMargin
            leftMargin: sideMargin
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
    PlasmaCore.FrameSvgItem {
        id: base
        anchors {
            fill: messageFlickable
            margins: -5
        }
        imagePath: "widgets/lineedit"
        prefix: "base"
    }
    Flickable {
        id: messageFlickable
        anchors {
            top: progress.bottom
            right: parent.right
            left: parent.left
            bottom: parent.bottom
            rightMargin: sideMargin
            leftMargin: sideMargin
            topMargin: 10
            bottomMargin: 10
        }
        clip: true
        contentHeight: message.height
        
        Label {
            id: message
            width: parent.width-messageScroll.width
            wrapMode: Text.WordWrap
            onTextChanged: {
                if(message.height>messageFlickable.height)
                    messageFlickable.contentY=message.height-messageFlickable.height
            }
        }
    }
}