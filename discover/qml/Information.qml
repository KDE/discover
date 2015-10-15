import QtQuick 2.1

PathView {
    id: viewItem
    interactive: false
    pathItemCount: count
    cacheItemCount: count
    highlightMoveDuration: 500
    readonly property real oriX: viewItem.width/2
    readonly property real oriY: viewItem.height/2
    property alias slideDuration: timer.interval
    
    path: Path {
        startX: oriX; startY: oriY
        PathLine { x: oriX-200; y: oriY }
        PathLine { x: oriX-800; y: oriY }
        PathLine { x: oriX+800; y: oriY-1900 }
        PathLine { x: oriX+800; y: oriY }
        PathLine { x: oriX+200; y: oriY }
        PathLine { x: oriX; y: oriY }
    }

    function next() { incrementCurrentIndex() }
    function previous() { decrementCurrentIndex() }
    
    onCurrentIndexChanged: timer.restart()
    Timer {
        id: timer
        interval: 5000; running: viewItem.visible
        onTriggered: viewItem.next()
    }
}
