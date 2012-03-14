import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Row {
    property bool editable: false
    property int max: 10
    property real rating: 2
    
    id: view
    height: 25
    
    Component {
        id: del
        QIconItem {
            height: view.height; width: view.height
            icon: "rating"
            opacity: (max/5*index)>rating ? 0.4 : 1

            MouseArea {
                enabled: editable
                
                anchors.fill: parent
                onClicked: rating = (max/5*index)
            }
        }
    }
    
    spacing: 2
    Repeater {
        model: 5
        delegate: del
    }
}