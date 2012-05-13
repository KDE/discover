import QtQuick 1.1
import org.kde.qtextracomponents 0.1
import org.kde.plasma.components 0.1

Item {
    property QtObject category: null
    
    QIconItem {
        id: iconItem
        icon: category.icon
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            margins: 15
        }
        width: height
    }
    
    Label {
        anchors {
            verticalCenter: parent.verticalCenter
            left: iconItem.right
            leftMargin: 50
        }
        font.pixelSize: parent.height*0.5
        text: category.name
    }
}