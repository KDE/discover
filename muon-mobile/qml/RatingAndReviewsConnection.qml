import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    property QtObject reviews
    
    function init() {
        reviews=app.appBackend.reviewsBackend()
    }
    
    Rectangle {
        id: bg
        anchors.fill: parent
        opacity: 0.4
    }
    
    Label {
        id: msg
        anchors {
            top: parent.top
            leftMargin: 5
            horizontalCenter: parent.horizontalCenter
        }
    }
    
    Column {
        visible: parent.state=="notlogged"
        anchors {
            top: msg.bottom
            horizontalCenter: parent.horizontalCenter
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
    
    ToolButton {
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
    
    state: (reviews && reviews.hasCredentials) ? "logged" : "notlogged"
    states: [
        State {
            name: "logged"
            PropertyChanges { target: msg; text: i18n("Signed in as <em>%1</em>", reviews.name) }
            PropertyChanges { target: bg; color: "orange" }
        },
        State {
            name: "notlogged"
            PropertyChanges { target: msg; text: i18n("Sign in is required") }
            PropertyChanges { target: bg; color: "blue" }
        }
    ]
}
