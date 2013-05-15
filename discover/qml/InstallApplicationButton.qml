import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    property alias application: listener.resource
    property alias isActive: listener.isActive
    property real maximumWidth: button.implicitWidth*2
    property Component additionalItem: null
    height: button.implicitHeight

    TransactionListener {
        id: listener
    }
    
    Button {
        id: button
        anchors {
            bottom: parent.bottom
            right: parent.right
        }
        visible: parent.state=="idle"
        text: application.isInstalled ? i18n("Install") : i18n("Remove")
        width: Math.min(parent.width/2, implicitWidth)
        
        onClicked: {
            if(application.isInstalled)
                resourcesModel.removeApplication(application);
            else
                resourcesModel.installApplication(application);
        }
    }
    Component {
        id: updateButton
        Button {
            text: i18n("Update")
            onClicked: resourcesModel.installApplication(application)
        }
    }
    
    Loader {
        visible: parent.state=="idle"
        anchors {
            verticalCenter: button.verticalCenter
            right: button.left
            left: parent.left
            rightMargin: 5
        }
        width: Math.min(parent.width/2, implicitWidth)
        sourceComponent: application.canUpgrade ? updateButton : additionalItem
    }
    
    Item {
        visible: parent.state=="working"
        anchors.fill: parent
        height: workingCancelButton.height
        
        Label {
            id: workingLabel
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                right: workingCancelButton.left
            }
            text: listener.statusText
        }
        
        Button {
            id: workingCancelButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            iconSource: "dialog-cancel"
            enabled: listener.isCancellable
            onClicked: resourcesModel.cancelTransaction(application)
        }
    }
    
    states: [
        State {
            name: "idle"
            when: !listener.isActive
        },
        State {
            name: "working"
            when: listener.isActive
        }
    ]
}
