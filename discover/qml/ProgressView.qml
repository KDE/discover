import QtQuick 1.1
import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

ToolBar {
    id: page
    property bool active: enabled && progressModel.count>0
    height: active ? contents.height+2*contents.anchors.margins : 0
    
    Behavior on height {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }
    
    Connections {
        target: transactionModel
        onTransactionAdded: {
            if(page.enabled && progressModel.appAt(trans.resource)<0)
                progressModel.append({'app': trans.resource})
        }

        onTransactionCancelled: {
            var id = progressModel.appAt(trans.resource)
            if(id>=0)
                progressModel.remove(id)
        }
    }
    
    ListModel {
        id: progressModel
        function appAt(app) {
            for(var i=0; i<progressModel.count; i++) {
                if(progressModel.get(i).app==app) {
                    return i
                }
            }
            return -1
        }
    }
    
    ListView {
        id: contents
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 3
        }
        
        spacing: 3
        height: 30
        orientation: ListView.Horizontal
        
        model: progressModel
        
        delegate: ListItem {
            width: launcherRow.childrenRect.width+5
            height: contents.height
            enabled: true
            onClicked: Navigation.openApplication(model.app)
            TransactionListener {
                id: listener
                resource: model.app
                onCancelled: model.remove(index)
            }
            
            Row {
                id: launcherRow
                spacing: 2
                IconItem { source: model.app.icon; height: parent.height; width: height }
                Label { text: model.app.name }
                Label { text: listener.statusText; visible: listener.isActive }
                ToolButton {
                    iconSource: "dialog-cancel"
                    visible: listener.isCancellable
                    onClicked: resourcesModel.cancelTransaction(app)
                }
                ToolButton {
                    iconSource: "system-run"
                    visible: model.app.isInstalled && !listener.isActive && model.app.canExecute
                    onClicked: {
                        model.app.invokeApplication()
                        model.remove(index)
                    }
                }
            }
        }
    }
    ToolButton {
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: 5
        }
        height: Math.min(implicitHeight, parent.height)
        iconSource: "window-close"
        onClicked: progressModel.clear()
    }
}
