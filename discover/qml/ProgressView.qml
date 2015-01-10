import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
import org.kde.muon 1.0
import "navigation.js" as Navigation

ToolBar {
    id: page
    property bool active: enabled && progressModel.count>0
    Layout.maximumHeight: active ? contents.height+2*contents.anchors.margins : 0
    
    Behavior on Layout.maximumHeight {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
    }
    
    Connections {
        target: TransactionModel
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

        delegate: Button {
            width: launcherRow.implicitWidth+launcherRow.anchors.margins*2
            height: contents.height

            onClicked: Navigation.openApplication(model.app)
            TransactionListener {
                id: listener
                resource: model.app
                onCancelled: model.remove(index)
            }

            Behavior on width { NumberAnimation { duration: 250 } }

            RowLayout {
                id: launcherRow
                anchors {
                    fill: parent
                    margins: 5
                }
                spacing: 2
                QIconItem {
                    anchors.verticalCenter: parent.verticalCenter
                    icon: model.app.icon
                    Layout.preferredHeight: parent.height*0.5
                    width: height
                }
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    text: model.app.name + (listener.isActive ? " "+listener.statusText : "")
                }
                ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "dialog-cancel"
                    visible: listener.isCancellable
                    onClicked: ResourcesModel.cancelTransaction(app)
                }
                ToolButton {
                    anchors.verticalCenter: parent.verticalCenter
                    iconName: "system-run"
                    visible: model.app.isInstalled && !listener.isActive && model.app.canExecute
                    onClicked: {
                        model.app.invokeApplication()
                        model.remove(index)
                    }
                }
            }
            Rectangle {
                anchors {
                    bottom: parent.bottom
                    left: parent.left
                    bottomMargin: 3
                    leftMargin: 3
                    rightMargin: 3
                }
                width: (parent.width - anchors.leftMargin - anchors.rightMargin)*(listener.progress/100)
                SystemPalette { id: theme }
                color: theme.buttonText
                height: 1
                opacity: 0.5
                visible: listener.isActive
            }
        }
    }
    ToolButton {
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
        }
        height: Math.min(implicitHeight, parent.height)
        iconName: "window-close"
        onClicked: progressModel.clear()
    }
}
