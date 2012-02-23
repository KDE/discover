import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    height: 30
    property QtObject reviews: app.appBackend.reviewsBackend()
    
    Rectangle {
        id: bg
        anchors.fill: parent
        opacity: 0.2
        color: "blue"
    }
    
    anchors {
        top: parent.top
        left: parent.left
        right: parent.right
    }
    
    Label {
        id: msg
        anchors {
            left: parent.left
            verticalCenter: parent.verticalCenter
            leftMargin: 5
        }
    }
    
    Row {
        visible: parent.state=="notlogged"
        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            rightMargin: 5
        }
        
        spacing: 10
        
        Button {
            text: i18n("Login")
            iconSource: "network-connect"
            onClicked: reviews.login()
        }
        Button {
            text: i18n("Register")
            iconSource: "system-users"
            onClicked: reviews.registerAndLogin();
        }
    }
    
    Button {
        visible: parent.state=="logged"
        anchors {
            verticalCenter: parent.verticalCenter
            right: parent.right
            rightMargin: 5
        }
        
        text: i18n("Logout")
        iconSource: "dialog-close"
        onClicked: reviews.logout();
    }
    
    state: reviews.hasCredentials ? "logged" : "notlogged"
    states: [
        State {
            name: "logged"
            PropertyChanges { target: msg; text: i18n("Signed in as <em>%1</em>", reviews.name) }
            PropertyChanges { target: bg; opacity: 0 }
        },
        State {
            name: "notlogged"
            PropertyChanges { target: msg; text: i18n("Sign in is required") }
            PropertyChanges { target: bg; opacity: 0.2 }
        }
    ]
}
