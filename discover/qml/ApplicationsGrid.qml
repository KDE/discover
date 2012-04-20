import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    id: parentItem
//     property alias header: view.header
    property string section//: view.section
    property alias delegate: gridRepeater.delegate
//     property Component delegate: null
    property alias model: gridRepeater.model
    
    property real cellWidth: Math.min(300, view.width)
    property real cellHeight: cellWidth/1.618 //tau
    
    Flickable {
        id: viewFlickable
        anchors.fill: parent
        contentHeight: view.height
        Flow
        {
            id: view
            spacing: 5
            width: parent.width-2*parent.width/8
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
            }
            
            Repeater {
                id: gridRepeater
                delegate: Rectangle { color: "red"; width: cellWidth; height: cellHeight }
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