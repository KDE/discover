import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.core 0.1 as PlasmaCore
import org.kde.muon 1.0

Page
{
    id: page
    property real actualWidth: width-Math.pow(width/70, 2)
    property real sideMargin: (width-actualWidth)/2
    
    function start() { updatesModel.updateAll() }
    ResourcesUpdatesModel {
        id: updatesModel
        resources: resourcesModel
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
        value: updatesModel.progress*100
        minimumValue: 0
        maximumValue: 100
    }
//     ScrollBar {
//         orientation: Qt.Vertical
//         flickableItem: messageFlickable
//         anchors {
//             top: parent.top
//             bottom: parent.bottom
//             right: parent.right
//         }
//         enabled: false
//     }
    PlasmaCore.FrameSvgItem {
        id: base
        anchors {
            fill: messageFlickable
            margins: -5
        }
        imagePath: "widgets/lineedit"
        prefix: "base"
    }
    ListView {
        id: messageFlickable
        property bool userScrolled: false
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
        model: updatesModel
        delegate: Label {
            text: display
            height: paintedHeight
            wrapMode: Text.Wrap
            width: messageFlickable.width
        }
        onContentHeightChanged: {
            if(!userScrolled && count>0 && !moving) {
                contentY = contentHeight - height + anchors.topMargin/2
            }
        }
        
        //if the user scrolls down, the viewport will be back to following the new progress
        onMovementEnded: userScrolled = !messageFlickable.atYEnd
    }
}