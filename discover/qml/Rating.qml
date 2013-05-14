import QtQuick 1.1
import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1

Row {
    id: view
    property bool editable: false
    property int max: 10
    property real rating: 2
    visible: rating>=0
    clip: true
    height: (width/5)-spacing
    width: 20*5
    spacing: 1
    
    Component {
        id: del
        IconItem {
            height: view.height; width: view.height
            source: "rating"
            opacity: (max/theRepeater.count*index)>rating ? 0.2 : 1

            MouseArea {
                enabled: editable
                
                anchors.fill: parent
                onClicked: rating = (max/5*index)
            }
        }
    }
    
    Repeater {
        id: theRepeater
        model: 5
        delegate: del
    }
}
