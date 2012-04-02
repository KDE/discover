import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    id: parentItem
    property int elemHeight: 65
    property alias count: view.count
    property alias header: view.header
    property string section//: view.section
    property alias delegate: view.delegate
    property alias cellWidth: view.cellWidth
    property alias cellHeight: view.cellHeight
    property alias model: view.model
    property int minCellWidth: 200
    
    GridView
    {
        id: view
        cellWidth: view.width/Math.floor(view.width/minCellWidth)-1
        cellHeight: cellWidth/1.618 //tau
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
            leftMargin: parent.width/8
            rightMargin: parent.width/8
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
        }
    }
}