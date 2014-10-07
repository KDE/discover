import QtQuick 2.1
import QtQuick.Controls 1.1
import org.kde.kquickcontrolsaddons 2.0

ToolButton
{
    id: button
    property string icon: ""
    property alias text: labelItem.text
    property alias overlayText: overlayTextItem.text
    width: height+(labelItem.text=="" ? 0 : labelItem.width)
    anchors.margins: 5
    
    QIconItem {
        id: iconItem
        anchors {
            top: parent.top
            bottom: parent.bottom
            left: parent.left
            margins: 5
        }
        width: height
        smooth: true
        icon: button.icon
    }
    
    Label {
        id: labelItem
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
            margins: 5
        }
    }
    
    Rectangle {
        visible: overlayText!=''
        color: "red"
        smooth: true
        anchors {
            bottom: parent.bottom
            right: parent.right
            rightMargin: 5
            bottomMargin: 5
        }
        width: overlayTextItem.width+6
        height: overlayTextItem.height
        radius: height
        border.color: "#77ffffff"
        border.width: 3
        Text {
            anchors.centerIn: parent
            id: overlayTextItem
            color: "white"
            font.bold: true
        }
    }
}
