import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    property alias application: transactions.application
    property bool canHide: parent.state=="idle"
    width: 100
    height: 30
    
    TransactionListener {
        id: transactions
        backend: app.appBackend
    }
    
    Button {
        id: button
        visible: parent.state=="idle"
        anchors.fill: parent
        
        onClicked: {
            switch(state) {
                case "willinstall": app.appBackend.installApplication(application); break;
                case "willremove":  app.appBackend.removeApplication(application); break;
            }
        }
        
        states: [
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
        
        Label {
            id: workingLabel
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            text: transactions.comment
        }
        ProgressBar {
            id: progress
            anchors {
                verticalCenter: parent.verticalCenter
                left: workingLabel.right
                right: workingCancelButton.left
            }
            
            minimumValue: 0
            maximumValue: 100
            value: transactions.progress
        }
        
        Button {
            id: workingCancelButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            iconSource: "dialog-cancel"
        }
    }
    
    states: [
        State {
            name: "idle"
            when: !transactions.isRunning
        },
        State {
            name: "working"
            when: transactions.isRunning
        }
    ]
}