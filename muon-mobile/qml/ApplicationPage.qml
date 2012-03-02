import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    id: page
    property QtObject application
    anchors.margins: 5
    
    ListView {
        id: reviewsView
        clip: true
        anchors.top: reviewsLabel.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: scroll.width
        spacing: 5
        delegate: ListItem {
            visible: model["shouldShow"]
            height: content.height+10
            
            function usefulnessToString(favorable, total)
            {
                return total==0
                        ? i18n("<em>Tell us about this review!</em>")
                        : i18n("<em>%1 out of %2 people found this review useful</em>", favorable, total)
            }
            
            Label {
                anchors.left: parent.left
                anchors.right: parent.right
                
                id: content
                text: i18n("<b>%1</b> by %2<p/>%3<br/>%4", summary, reviewer,
                           display, usefulnessToString(usefulnessFavorable, usefulnessTotal))
                wrapMode: Text.WordWrap
            }
            
            Label {
                visible: app.appBackend.reviewsBackend().hasCredentials
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                opacity: delegateArea.containsMouse ? 1 : 0.2
                
                text: i18n("<em>Useful? <a href='true'>Yes</a>/<a href='false'>No</a></em>")
                onLinkActivated: reviewsModel.markUseful(index, link)
            }
            
            Rating {
                anchors.top: parent.top
                anchors.right: parent.right
                rating: model["rating"]
                height: content.font.pixelSize
            }
            MouseArea {
                id: delegateArea
                anchors.fill: parent
                hoverEnabled: true
            }
        }
        
        model: ReviewsModel {
            id: reviewsModel
            application: page.application
            backend: app.appBackend.reviewsBackend()
        }
    }
    
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
            anchors.rightMargin: 5
            width: 40
            height: 40
            
            icon: application.icon
        }
        
        Column {
            id: header
            anchors.top: parent.top
            anchors.left: icon.right
            anchors.right: ratings.left
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
                            +"<b>Version:</b> %2<br/>"
                            +"<b>License:</b> %3<br/>",
                            application.sizeDescription,
                            application.name+" "+(application.isInstalled ?
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
    
    Label {
        id: reviewsLabel
        anchors.top: appInfo.bottom
        text: "<b>Reviews:</b>" 
        visible: reviewsView.count>0
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: reviewsView
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
            top: reviewsView.top
            right: parent.right
            bottom: reviewsView.bottom
        }
    }
}