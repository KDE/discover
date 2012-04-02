import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    id: parentItem
    property Item stack
    property alias sortRole: apps.stringSortRole
    property alias sortOrder: apps.sortOrder
    property int elemHeight: 65
    property alias stateFilter: apps.stateFilter
    property alias count: view.count
    property alias header: view.header
    property string section//: view.section
    property alias category: apps.filteredCategory
    property bool preferUpgrade: false
    property alias delegate: view.delegate
    property alias cellWidth: view.cellWidth
    property alias cellHeight: view.cellHeight
    property int minCellWidth: 200

    function searchFor(text) { apps.search(text); apps.sortOrder=Qt.AscendingOrder }
    function stringToRole(role) { return apps.stringToRole(role) }
    function roleToString(role) { return apps.roleToString(role) }
    function applicationAt(i) { return apps.applicationAt(i) }
    
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
        
        model: ApplicationProxyModel {
            id: apps
            stringSortRole: "ratingPoints"
            sortOrder: Qt.DescendingOrder
            
            Component.onCompleted: sortModel()
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