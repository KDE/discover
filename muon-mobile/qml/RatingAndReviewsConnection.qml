import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Item {
    id: page
    property QtObject reviews
    width: contents.width+2*contents.anchors.margins
    height: contents.height+2*contents.anchors.margins
    
    function init() {
        reviews=app.appBackend.reviewsBackend()
    }
    
    Rectangle {
        id: bg
        anchors.fill: parent
        opacity: 0.4
        radius: 10
    }
    
    Column {
        id: contents
        spacing: 10
        anchors.margins: 10
        anchors.centerIn: parent
        Label {
            id: msg
        }
        
        Button {
            anchors.horizontalCenter: contents.horizontalCenter
            visible: page.state=="notlogged"
            text: i18n("Login")
            iconSource: "network-connect"
            onClicked: reviews.login()
        }
        Button {
            anchors.horizontalCenter: contents.horizontalCenter
            visible: page.state=="notlogged"
            text: i18n("Register")
            iconSource: "system-users"
            onClicked: reviews.registerAndLogin();
        }
        Button {
            anchors.horizontalCenter: contents.horizontalCenter
            visible: page.state=="logged"
            
            text: i18n("Logout")
            iconSource: "dialog-close"
            onClicked: reviews.logout();
        }
    }
    
    onVisibleChanged: opacity=(visible? 1 : 0)
    
    Behavior on opacity {
        NumberAnimation { duration: 250; easing.type: Easing.InOutQuad }
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
