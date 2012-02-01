import QtQuick 1.1
import org.kde.plasma.components 0.1

ListView
{
    id: view
    signal clicked(int idx)
    
    function pushItem(icon, text) {
        items.append({"decoration": icon, "display": text})
    }
    
    function popItem() {
        items.remove(items.count-1)
    }
    
    spacing: 10
    model: items
    layoutDirection: Qt.LeftToRight
    orientation: ListView.Horizontal
    delegate: ToolButton {
        height: 30
        iconSource: decoration
        anchors.verticalCenter: parent.verticalCenter
        onClicked: view.clicked(count-index-1)
        text: display
    }
    
    onCountChanged: view.positionViewAtEnd()
    
    ListModel {
        id: items
        ListElement { decoration: "kalgebra"; display: "lalala" }
    }
}
