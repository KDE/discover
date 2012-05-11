import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import "Helpers.js" as Helpers

Item {
    id: topItem
    
    Information {
        id: info
        anchors.centerIn: parent
        height: parent.height-10
        width: parent.width
        Connections {
            target: app.appBackend
            onAppBackendReady: Helpers.getFeatured(noCategoryModel, info.featuredData)
        }
        Component.onCompleted: {
            var xhr = new XMLHttpRequest;
            xhr.open("GET", app.featuredSource());
            xhr.onreadystatechange = function() {
                if (xhr.readyState == XMLHttpRequest.DONE) {
                    featuredData = JSON.parse(xhr.responseText)
                }
            }
            xhr.send();
        }
        property variant featuredData: null
        onFeaturedDataChanged: Helpers.getFeatured(noCategoryModel, featuredData)
        
        ListModel {
            id: noCategoryModel
            ListElement { text: "Kubuntu"; color: "#84D1FF"; icon: "kde"; comment: ""; image: "http://www.kubuntu.org/files/12.04-lts-banner.png"; packageName: "" }
        }
        
        dataModel: noCategoryModel
        
        delegate: MouseArea {
                property QtObject modelData
                id: infoArea
                enabled: modelData.package!=""
                anchors.fill: parent
                onClicked: Navigation.openApplication(app.appBackend.applicationByPackageName(modelData.packageName))
                clip: true
                
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: modelData.color
                    opacity: 0.3
                    
                    Behavior on opacity { NumberAnimation { duration: 500 } }
                }
                Flickable {
                    id: flick
                    anchors.fill: parent
                    contentY: 0
                    interactive: false
                    contentWidth: image.width; contentHeight: image.height
                    Behavior on contentY { NumberAnimation { duration: 5000 } }
                    
                    Image {
                        id: image
                        source: modelData.image
                        
                        onStatusChanged: {
                            if(status==Image.Ready && image.height > flick.height) {
                                flick.contentY=flick.contentHeight-flick.height
                            }
                        }
                    }
                }
                Item {
                    height: 40
                    
                    anchors {
                        left: parent.left
                        right: parent.right
                        bottom: parent.bottom
                    }
                    
                    Rectangle {
                        anchors.fill: parent
                        color: "black"
                        opacity: 0.7
                    }
                    
                    ToolButton {
                        id: prevButton
                        iconSource: "go-previous"
                        height: parent.height
                        onClicked: info.previous()
                        anchors {
                            top: parent.top
                            left: parent.left
                        }
                    }
                    
                    QIconItem {
                        id: iconItem
                        anchors {
                            left: prevButton.right
                            top: parent.top
                            bottom: parent.bottom
                            margins: 3
                        }
                        width: height
                        icon: modelData.icon
                    }
                    
                    Label {
                        anchors {
                            left: iconItem.right
                            verticalCenter: parent.verticalCenter
                            leftMargin: 10
                        }
                        color: "white"
                        text: i18n("<b>%1</b><br/>%2", modelData.text, modelData.comment)
                    }
                    ToolButton {
                        iconSource: "go-next"
                        height: parent.height
                        onClicked: info.next()
                        anchors {
                            right: parent.right
                            top: parent.top
                        }
                    }
                }
        }
    }
}