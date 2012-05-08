import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: viewItem
    property int current: 0
    property QtObject dataModel
    property QtObject currentElement: dataModel.get(current)
    property Component delegate
    property Item currentItem
    visible: false
    smooth: true
    
    onDelegateChanged: restoreView()
    onCurrentElementChanged: restoreView()
    onDataModelChanged: current = 0
    
    Component.onCompleted: viewItem.visible=true
    
    function restoreView() { return restoreViewInternal(!viewItem.visible); }
    function restoreViewInternal(force) {
        if(!delegate || (destroyAnimation.running && !force) || !currentElement) {
            return
        }
        
        try {
            var oldItem = viewItem.currentItem
            viewItem.currentItem = delegate.createObject(viewItem, { "modelData": currentElement })
            viewItem.currentItem.anchors.fill=viewItem
            
            if(oldItem) {
                if(force) {
                    oldItem.destroy()
                } else {
                    viewItem.currentItem.opacity = 0
                    oldItem.z = viewItem.currentItem.z+1
                    fadeoutAnimation.target = oldItem
                    fadeinAnimation.target = viewItem.currentItem
                    destroyAnimation.start()
                }
            }
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+delegate.errorString())
        }
        if(timer.running)
            timer.restart()
    }
    
    property real endOfWindow: 1230
    ParallelAnimation {
        id: destroyAnimation
        NumberAnimation {
            id: fadeoutAnimation
            duration: 500
            to: viewItem.endOfWindow
            property: "anchors.leftMargin"
            easing.type: Easing.InQuad
            onCompleted: target.destroy()
        }
        NumberAnimation {
            duration: 500
            to: -viewItem.endOfWindow
            property: "anchors.rightMargin"
            target: fadeoutAnimation.target
            easing.type: fadeoutAnimation.easing.type
        }
        NumberAnimation {
            duration: 500
            from: 1
            to: 0
            property: "opacity"
            target: fadeoutAnimation.target
            easing.type: Easing.InQuad
        }
        NumberAnimation {
            id: fadeinAnimation
            duration: 500
            from: 0
            to: 1
            property: "opacity"
            target: viewItem.currentItem
            easing.type: Easing.InQuad
        }
    }
    
    function next() {
        viewItem.current = (viewItem.current+1)%dataModel.count
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: info.next()
    }
    
    Timer {
        id: timer
        interval: 5000; running: viewItem.visible; repeat: true
        onTriggered: info.next()
    }
    
    Row {
        id: selectorRow
        anchors {
            horizontalCenter: parent.horizontalCenter
            bottom: parent.bottom
        }
        
        width: 100
        height: 20
        spacing: 10
        z: currentItem ? currentItem.z+2 : 0
        visible: viewItem.dataModel.count>1

        Repeater {
            model: viewItem.dataModel.count
            
            Rectangle {
                property bool isCurrent: modelData == current
                anchors.verticalCenter: parent.verticalCenter
                width:  isCurrent ? 15 : 10
                height: width
                radius: width
                smooth: true
                color: "black"
                border.color: "white"
                border.width: 2
                opacity: isCurrent ? 1 : 0.2
                
                Behavior on opacity { NumberAnimation { duration: 250 } }
                Behavior on width { NumberAnimation { duration: 250 } }
                
                MouseArea {
                    id: area
                    anchors.fill: parent
                    onClicked: current = modelData
                }
            }
        }
    }
}