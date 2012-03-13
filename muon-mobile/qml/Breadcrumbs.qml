import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: bread
    signal clicked(int idx)
    property alias count: items.count
    property Item tools: toolbar
    anchors.margins: 5
    
    function currentItem() {
        return items.count=="" ? null : items.get(items.count-1).display
    }
    
    function pushItem(icon, text) {
        items.append({"decoration": icon, "display": text})
    }
    
    function popItem() {
        items.remove(items.count-1)
    }
    
    ListView
    {
        id: view
        anchors {
            fill: parent
        }
        
        spacing: 10
        model: items
        layoutDirection: Qt.LeftToRight
        orientation: ListView.Horizontal
        delegate: ToolButton {
            height: bread.height
            iconSource: decoration
            onClicked: bread.clicked(items.count-index-1)
            text: display ? display : ""
        }
        
        onCountChanged: view.positionViewAtEnd()
        
        ListModel { id: items }
    }
}