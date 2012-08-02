import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import "Helpers.js" as Helpers

Information {
    id: info
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
    Connections {
        target: resourcesModel
        onRowsInserted: {
            //TODO: Inefficient?
            if(info.featuredData!=null) {
                Helpers.getFeatured(noCategoryModel, featuredData)
            }
        }
    }
    
    model: ListModel {
        id: noCategoryModel
//         ListElement { text: "Kubuntu"; color: "#84D1FF"; icon: "kde"; comment: ""; image: "http://www.kubuntu.org/files/12.04-lts-banner.png"; packageName: "" }
    }
    
    delegate: MouseArea {
            property QtObject modelData: model
            enabled: modelData.package!=""
            width: Math.min(image.width, parent.width); height: parent.height
            onClicked: Navigation.openApplication(resourcesModel.resourceByPackageName(modelData.packageName))
            
            clip: true
            z: PathView.isCurrentItem && !PathView.view.moving ? 1 : -1
            id: itemDelegate
            
            states: [
                State {
                    name: "shownSmall"
                    when: image.status==Image.Ready && image.height<(flick.height-titleBar.height)
                    PropertyChanges { target: flick; contentY: (flick.contentHeight+titleBar.height)/2-flick.height/2 }
                },
                State {
                    name: "shownIdeal"
                    when: image.status==Image.Ready && image.height<flick.height
                    PropertyChanges { target: flick; contentY: image.height-height }
                },
                State {
                    name: "shownBig"
                    when: itemDelegate.PathView.isCurrentItem
                    PropertyChanges { target: flick; contentY: flick.contentHeight-flick.height }
                },
                State {
                    name: "notShown"
                    when: !itemDelegate.PathView.isCurrentItem
                    PropertyChanges { target: flick; contentY: 0 }
                }
            ]
            transitions: [
                Transition {
                    from: "notShown"; to: "shownBig"
                    NumberAnimation {
                        properties: "contentY"
                        duration: info.slideDuration
                        easing.type: Easing.InOutQuad
                    }
                },
                Transition {
                    to: "notShown"; from: "shownBig"
                    NumberAnimation {
                        properties: "contentY"
                        duration: info.slideDuration
                        easing.type: Easing.InOutQuad
                    }
                }
            ]
            
            Flickable {
                id: flick
                anchors.centerIn: parent
                height: parent.height
                width: parent.width
                
                contentY: 0
                interactive: false
                contentX: contentWidth < width ? (contentWidth-width)/2 : 0
                contentWidth: Math.ceil(image.width, width); contentHeight: Math.ceil(image.height, height)
                
                Image {
                    id: image
                    source: modelData.image
                }
            }
            Item {
                id: titleBar
                height: 40
                
                anchors {
                    left: flick.left
                    right: flick.right
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
