import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

Item {
    height: 100
    property QtObject category
    
    QIconItem {
        id: iconItem
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
        }
        height: parent.height*0.7; width: height
        icon: category.icon
    }
    
    Text {
        anchors {
            left: iconItem.right
            verticalCenter: parent.verticalCenter
            leftMargin: font.pointSize/2
        }
        text: category.name
        font.pointSize: 40
    }
}