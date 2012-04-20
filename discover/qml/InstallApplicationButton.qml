import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    id: item
    property alias application: transactions.application
    property bool canHide: parent.state=="idle"
    property bool preferUpgrade: false
    property alias isInstalling: transactions.isActive
    width: 100
    height: 30
    
    TransactionListener {
        id: transactions
        backend: app.appBackend
    }
    
    Button {
        id: button
        visible: parent.state=="idle"
        width: parent.width
        anchors.fill: parent
        
        onClicked: {
            switch(state) {
                case "willupgrade":
                case "willinstall": app.appBackend.installApplication(application); break;
                case "willremove":  app.appBackend.removeApplication(application); break;
            }
        }
        
        states: [
            State {
                name: "willupgrade"
                when: application.canUpgrade && item.preferUpgrade
                PropertyChanges { target: button;  text: i18n("Upgrade") }
            },
            State {
                name: "willinstall"
                when: !application.isInstalled
                PropertyChanges { target: button;  text: i18n("Install") }
            },
            State {
                name: "willremove"
                when: application.isInstalled
                PropertyChanges { target: button;  text: i18n("Remove") }
            }
        ]
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
            text: transactions.comment
        }
        
        Button {
            id: workingCancelButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            iconSource: "dialog-cancel"
            enabled: transactions.isDownloading
            onClicked: app.appBackend.cancelTransaction(application)
        }
    }
    
    states: [
        State {
            name: "idle"
            when: !transactions.isActive
        },
        State {
            name: "working"
            when: transactions.isActive
        }
    ]
}