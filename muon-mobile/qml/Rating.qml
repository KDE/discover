import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Row {
    id: view
    height: 25
    
    Component {
        id: del
        QIconItem {
            height: view.height; width: view.height
            icon: "rating"
            opacity: (max/5*index)>rating ? 0.4 : 1
        }
    }
    
    property int max: 10
    property real rating: 2
    
    spacing: 2
    Repeater {
        model: 5
        delegate: del
    }
}