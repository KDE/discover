import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    property alias application: transactions.application
    width: 100
    height: 30
    
    TransactionListener {
        id: transactions
        backend: app.appBackend
    }
    
    state: transactions.isInstalling ? "working" : "idle"
    
    Button {
        id: button
        state: application.isInstalled ? "willremove" : "willinstall"
        visible: parent.state=="idle"
        anchors.fill: parent
        
        onClicked: {
            if(state=="willinstall") app.appBackend.installApplication(application)
            else if(state=="willremove") app.appBackend.removeApplication(application)
        }
        
        states: [
            State {
                name: "willinstall"
                PropertyChanges { target: button;  text: i18n("Install") }
            },
            State {
                name: "willremove"
                PropertyChanges { target: button;  text: i18n("Remove") }
            }
        ]
    }
    
    Row {
        visible: parent.state=="working"
        ProgressBar {
            id: progress
            minimumValue: 0
            maximumValue: 100
            value: transactions.progress
            
            Label {
                anchors.fill: parent
                text: transactions.comment
            }
        }
        
        Button { iconSource: "dialog-cancel" }
    }
    
    states: [
        State { name: "idle" },
        State { name: "working" }
    ]
}