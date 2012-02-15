import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    id: page
    property QtObject application
    anchors.margins: 10
    
    Row {
        id: header
        spacing: 20
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        QIconItem {
            id: icon
            width: 40
            height: 40
            
            icon: application.icon
        }
        
        Column {
            Label {
                font.bold: true
                text: application.name
            }
            Label { text: application.comment }
        }
        Column {
            Rating { rating: app.appBackend.reviewsBackend().ratingForApplication(application).rating() }
            Label { text: i18n("%1 reviews", app.appBackend.reviewsBackend().ratingForApplication(application).ratingCount()) }
        }
    }
        
    Image {
        id: screenshot
        width: 200
        anchors.top: parent.top
        anchors.right: parent.right
        
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        source: application.screenshotUrl(state == "thumbnail" ? 0 : 1)
        state: "thumbnail"
        
        states: [
            State { name: "thumbnail"
                PropertyChanges { target: screenshot; height: 100 }
            },
            State { name: "full"
                PropertyChanges { target: screenshot; height: parent.height }
                PropertyChanges { target: screenshot; width: parent.width }
                PropertyChanges { target: screenshot; z: 1 }
            }
        ]
        
        transitions: Transition {
            NumberAnimation { properties: "height,width"; easing.type: Easing.InOutBack; duration: 500 }
        }
        
        MouseArea {
            anchors.fill: screenshot
            onClicked: { screenshot.state= screenshot.state == "thumbnail" ? "full" : "thumbnail" }
        }
    }
    
    Column {
        id: info
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10
        
        InstallApplicationButton {
            application: page.application
        }
        
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: i18n("<b>Description:</b><br/>%1", application.longDescription)
        }
        
        Button {
            text: i18n("Homepage")
            iconSource: "go-home"
            enabled: application.homepage
            onClicked: app.openUrl(application.homepage)
        }
        
        Label {
            text: i18n(  "<b>Total Size:</b> %1<br/>"
                        +"<b>Version:</b> %2<br/>"
                        +"<b>License:</b> %3<br/>",
                         application.sizeDescription,
                         application.name+" "+(application.isInstalled ?
                                                    application.installedVersion : application.availableVersion),
                         application.license
            )
        }
        
        Label { text: "Reviews" }
    }
    
    ListView {
        id: reviewsView
        clip: true
        anchors.top: info.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 5
        delegate: ListItem {
//             Column {
                Label {
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    text: display
                    wrapMode: Text.WordWrap
                }
//             }
        }
        
        model: ReviewsModel {
            application: page.application
            backend: app.appBackend.reviewsBackend()
        }
        
        ScrollBar {
                orientation: Qt.Vertical
                flickableItem: reviewsView
                stepSize: 40
                scrollButtonInterval: 50
                anchors {
                        top: reviewsView.top
                        right: reviewsView.right
                        bottom: reviewsView.bottom
                }
        }
    }
}