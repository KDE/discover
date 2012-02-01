import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: bread
    signal clicked(int idx)
    property alias search: searchInput.text
    
    function pushItem(icon, text, withsearch) {
        items.append({"decoration": icon, "display": text, "withsearch": withsearch})
    }
    
    function popItem() {
        items.remove(items.count-1)
        searchInput.text=""
    }
    
    ListView
    {
        id: view
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            right: searchInput.left
        }
        
        spacing: 10
        model: items
        layoutDirection: Qt.LeftToRight
        orientation: ListView.Horizontal
        delegate: ToolButton {
            height: 30
            width: implicitWidth
            iconSource: decoration
            anchors.verticalCenter: parent.verticalCenter
            onClicked: bread.clicked(items.count-index-1)
            text: display
        }
        
        onCountChanged: view.positionViewAtEnd()
        
        ListModel { id: items }
    }
    
    TextField {
        id: searchInput
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
        }
        width: 80
        placeholderText: i18n("Search...")
        visible: items.count>0 && items.get(items.count-1).withsearch
    }
}