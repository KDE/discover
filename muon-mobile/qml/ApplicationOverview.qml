import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Item {
    id: appInfo
    anchors.top: parent.top
    anchors.left: parent.left
    anchors.right: parent.right
    height: Math.max(header.height+info.height+55, 100+ratings.height)
    
    QIconItem {
        id: icon
        anchors.top: parent.top
        anchors.left: parent.left
        width: 40
        height: 40
        
        icon: application.icon
    }
    
    Column {
        id: header
        anchors.top: parent.top
        anchors.left: icon.right
        anchors.right: ratings.left
        anchors.leftMargin: 5
        spacing: 5
        
        Text {
            text: application.name
            width: parent.width
            font.bold: true
        }
        Label {
            text: application.comment
            wrapMode: Text.WordWrap
            width: parent.width
        }
    }
    
    Column {
        id: ratings
        spacing: 10
        anchors.top: installButton.bottom
        anchors.right: parent.right
        
        Rating { 
            anchors.horizontalCenter: parent.horizontalCenter
            rating: app.appBackend.reviewsBackend().ratingForApplication(application).rating()
        }
        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n("%1 reviews", app.appBackend.reviewsBackend().ratingForApplication(application).ratingCount())
        }
        
        
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            text: i18n("Homepage")
            iconSource: "go-home"
            enabled: application.homepage
            onClicked: app.openUrl(application.homepage)
        }
        
        Button {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: application.isInstalled
            text: i18n("Review")
            onClicked: reviewDialog.open()
        }
        
        Label {
            text: i18n(  "<b>Total Size:</b> %1<br/>"
                        +"<b>Version:</b> %2 %3<br/>"
                        +"<b>License:</b> %4<br/>",
                        application.sizeDescription,
                        application.name, (application.isInstalled ?
                                                    application.installedVersion : application.availableVersion),
                        application.license
            )
        }
    }
    
    ReviewDialog {
        id: reviewDialog
        application: page.application
        onAccepted: app.appBackend.reviewsBackend().submitReview(page.application, summary, review, rating)
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
                PropertyChanges { target: screenshot; height: icon.height+installButton.height }
            },
            State { name: "full"
                PropertyChanges { target: screenshot; height: Math.min(page.height, sourceSize.height) }
                PropertyChanges { target: screenshot; width: Math.min(page.width, sourceSize.width) }
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
    
    InstallApplicationButton {
        id: installButton
        anchors.left: parent.left
        anchors.top: header.bottom
        application: page.application
    }
    
    Label {
        id: info
        anchors.top: installButton.bottom
        anchors.left: parent.left
        anchors.right: ratings.left
        anchors.margins: 5
        wrapMode: Text.WordWrap
        text: application.longDescription
    }
}