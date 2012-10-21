import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: parentItem
    property Component header: null
    property alias delegate: gridRepeater.delegate
    property alias model: gridRepeater.model
    
    property real actualWidth: width
    property real cellWidth: Math.min(200, actualWidth)
    property real cellHeight: cellWidth/1.618 //tau
    
    Flickable {
        id: viewFlickable
        anchors.fill: parent
        contentHeight: view.height+headerLoader.height
        
        Loader {
            id: headerLoader
            sourceComponent: parentItem.header
            anchors {
                left: view.left
                right: view.right
                top: parent.top
            }
        }
        
        Flow
        {
            id: view
            width: Math.floor(actualWidth/cellWidth)*(cellWidth+spacing)
            spacing: 5
            anchors {
                top: headerLoader.bottom
                horizontalCenter: parent.horizontalCenter
            }
            
            Repeater {
                id: gridRepeater
            }
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: viewFlickable
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
    }
}
