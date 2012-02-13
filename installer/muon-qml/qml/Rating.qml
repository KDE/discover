import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Row {
    Component {
        id: del
        
        QIconItem {
            height: 20; width: 20
            icon: "rating"
            opacity: index>rating ? 0.4 : 1
        }
    }
    
    id: view
    property int max: 5
    property real rating: 2
    
    spacing: 10
    Repeater {
        model: view.max
        delegate: del
    }
}