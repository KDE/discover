import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Row {
    Component {
        id: del
        
        QIconItem {
            height: 20; width: 20
            icon: "rating"
            opacity: (max/5*index)>rating ? 0.4 : 1
        }
    }
    
    id: view
    property int max: 10
    property real rating: 2
    
    spacing: 2
    Repeater {
        model: 5
        delegate: del
    }
}