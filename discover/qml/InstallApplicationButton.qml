import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0

Item {
    property alias application: listener.resource
    property alias isActive: listener.isActive
    property real maximumWidth: button.Layout.preferredWidth*2
    property real minimumWidth: button.Layout.minimumWidth*2
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
        enabled: !ResourcesModel.isFetching
        text: !application.isInstalled ? i18n("Install") : i18n("Remove")
        width: Math.min(parent.width/2, implicitWidth)
        
        onClicked: {
            if(application.isInstalled)
                ResourcesModel.removeApplication(application);
            else
                ResourcesModel.installApplication(application);
        }
    }
    Component {
        id: updateButton
        Button {
            text: i18n("Update")
            onClicked: ResourcesModel.installApplication(application)
        }
    }
    
    Loader {
        visible: parent.state=="idle"
        anchors {
            verticalCenter: button.verticalCenter
            left: parent.left
            rightMargin: 5
        }
        width: Math.min(parent.width/2-5, button.implicitWidth)
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
            iconName: "dialog-cancel"
            enabled: listener.isCancellable
            onClicked: ResourcesModel.cancelTransaction(application)
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
