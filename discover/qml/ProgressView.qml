import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

ToolBar {
    id: page
    property bool active: transactionsModel.count>0
    property bool enabled: true
    height: active ? contents.height+2*contents.anchors.margins : 0
    
    Behavior on height {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }
    
    Connections {
        id: backendConnections
        target: resourcesModel
        onTransactionAdded: {
            if(page.enabled && transactionsModel.appAt(transaction.resource)<0)
                transactionsModel.append({'app': transaction.resource})
        }

        onTransactionCancelled: {
            var id = transactionsModel.appAt(transaction.resource)
            if(id>=0)
                transactionsModel.remove(id)
        }
    }
    
    ListModel {
        id: transactionsModel
        function appAt(app) {
            for(var i=0; i<transactionsModel.count; i++) {
                if(transactionsModel.get(i).app==app) {
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
        
        model: transactionsModel
        
        delegate: ListItem {
            width: launcherRow.childrenRect.width+5
            height: contents.height
            enabled: true
            onClicked: Navigation.openApplication(model.app)
            TransactionListener {
                id: listener
                resource: model.app
                backend: model.app.backend
                onCancelled: model.remove(index)
            }
            
            Row {
                id: launcherRow
                spacing: 2
                QIconItem { icon: model.app.icon; height: parent.height; width: height }
                Label { text: model.app.name }
                Label { text: listener.comment; visible: listener.isActive }
                ToolButton {
                    iconSource: "dialog-cancel"
                    visible: listener.isDownloading
                    onClicked: resourcesModel.cancelTransaction(application)
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
        iconSource: "dialog-close"
        onClicked: transactionsModel.clear()
    }
}
