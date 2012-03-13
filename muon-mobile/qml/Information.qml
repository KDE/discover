import QtQuick 1.1
import org.kde.plasma.components 0.1

Item {
    id: view
    property int current: 0
    property QtObject data
    property QtObject currentElement: data.get(current)
    property Component delegate
    property Item currentItem
    
    onCurrentElementChanged: restoreView()
    onDelegateChanged: restoreView()
    
    function restoreView() {
        if(!delegate || !currentElement)
            return
        try {
            if(view.currentItem)
                view.currentItem.destroy()
            view.currentItem = delegate.createObject(view, { "modelData": currentElement })
            view.currentItem.anchors.fill=view
        } catch (e) {
            console.log("error: "+e)
            console.log("comp error: "+delegate.errorString())
        }
    }
    
    function next() {
        view.current = (view.current+1)%data.count
    }
}