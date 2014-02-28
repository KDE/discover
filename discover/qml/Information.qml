import QtQuick 1.1
import org.kde.plasma.components 0.1

PathView {
    id: viewItem
    interactive: false
    pathItemCount: count
    highlightMoveDuration: 500
    property real delWidth: width
    property real delHeight: height
    property alias slideDuration: timer.interval

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.05
    }
    
    path: Path {
        startX: delWidth/2; startY: delHeight/2
        PathLine { x: 3*delWidth; y: delHeight/2 }
        PathLine { x: 3*delWidth; y: -delHeight }
        PathLine { x: -3*delWidth; y: -delHeight }
        PathLine { x: -3*delWidth; y: delHeight/2 }
        PathLine { x: delWidth/2; y: delHeight/2 }
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
