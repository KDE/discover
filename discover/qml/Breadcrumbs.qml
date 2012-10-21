import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: bread
    property alias count: items.count
    property Item pageStack: null
    
    signal poppedPages
    
    function currentItem() {
        return items.count=="" ? null : items.get(items.count-1).display
    }
    
    function pushItem(icon, text) {
        items.append({"decoration": icon, "display": text})
    }
    
    function popItem(last) {
        items.remove(items.count-1)
        var page = pageStack.pop(undefined, last)
        page.destroy(1000)
        
        if(last)
            poppedPages()
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
            top: parent.top
            bottom: parent.bottom
            right: parent.right
            left: parent.left
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
}
