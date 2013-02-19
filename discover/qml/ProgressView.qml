import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

ToolBar {
    id: page
    property bool enabled: true
    property bool active: transactionModel.count>0
    height: active ? contents.height+2*contents.anchors.margins : 0

    Behavior on height {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
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

        model: transactionModel

        delegate: ListItem {
            width: launcherRow.childrenRect.width+5
            height: contents.height
            enabled: true
            onClicked: Navigation.openApplication(model.resource)
            TransactionListener {
                id: listener
                resource: model.resource
            }

            Row {
                id: launcherRow
                spacing: 2
                QIconItem { icon: model.resource.icon; height: parent.height; width: height }
                Label { text: model.resource.name }
                Label { text: listener.statusText; visible: listener.isActive }
                ToolButton {
                    iconSource: "dialog-cancel"
                    visible: listener.isCancellable
                    onClicked: resourcesModel.cancelTransaction(application)
                }
                ToolButton {
                    iconSource: "system-run"
                    visible: model.resource.isInstalled && !listener.isActive && model.resource.canExecute
                    onClicked: {
                        model.resource.invokeApplication()
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
        onClicked: transactionModel.clear()
    }
}
