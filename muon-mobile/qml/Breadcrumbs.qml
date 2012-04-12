import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: bread
    property alias count: items.count
    property Item pageStack: null
    anchors.margins: 5
    
    function currentItem() {
        return items.count=="" ? null : items.get(items.count-1).display
    }
    
    function pushItem(icon, text) {
        items.append({"decoration": icon, "display": text})
    }
    
    function popItem(last) {
        items.remove(items.count-1)
        pageStack.pop(undefined, last)
    }
    
    function doClick(index) {
        var pos = items.count-index-1
        for(; pos>0; --pos) {
            bread.popItem(pos>1)
        }
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
            onClicked: doClick(index)
            text: display ? display : ""
            visible: items.count-index>1
        }
        
        onCountChanged: view.positionViewAtEnd()
        
        ListModel { id: items }
    }
    
    ToolButton {
        iconSource: "go-up"
        anchors.right: parent.right
        height: parent.height
        visible: items.count>1
        onClicked: popItem(false)
    }
}