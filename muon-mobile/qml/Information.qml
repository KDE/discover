import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: viewItem
    property int current: 0
    property QtObject dataModel
    property QtObject currentElement: dataModel.get(current)
    property Component delegate
    property Item currentItem
    
    smooth: true
    onCurrentElementChanged: restoreView()
    onDelegateChanged: restoreView()
    
    function restoreView() {
        if(!delegate || !currentElement)
            return
        try {
            if(viewItem.currentItem)
                viewItem.currentItem.destroy()
            viewItem.currentItem = delegate.createObject(viewItem, { "modelData": currentElement })
            viewItem.currentItem.anchors.fill=viewItem
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+delegate.errorString())
        }
    }
    
    function next() {
        viewItem.current = (viewItem.current+1)%dataModel.count
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: info.next()
    }
    
    Row {
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        
        width: 100
        height: 20
        spacing: 10
        
        Repeater {
            model: viewItem.dataModel.count
            
            Rectangle {
                width: 10
                height: 10
                radius: 10
                smooth: true
                color: "black"
                opacity: area.containsMouse ? 0.2 : 0.6
                
                Behavior on opacity {
                    NumberAnimation { duration: 250 }
                }
                
                MouseArea {
                    id: area
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: current = modelData
                }
            }
        }
    }
}