import QtQuick 2.1
import QtQuick.Controls 1.1
import "navigation.js" as Navigation

Information {
    id: info
    model: FeaturedModel {}
    
    delegate: MouseArea {
            property QtObject modelData: model
            enabled: modelData.package!=""
            width: Math.min(flick.width, parent.width)
            height: parent.height
            
            onClicked: {
                if(modelData.packageName!=null)
                    Navigation.openApplication(ResourcesModel.resourceByPackageName(modelData.packageName))
                else
                    Qt.openUrlExternally(modelData.url)
            }
            
            clip: true
            z: PathView.isCurrentItem && !PathView.view.moving ? 1 : -1
            id: itemDelegate
            
            Loader {
                id: flick
                anchors.centerIn: parent
                
                function endsWith(str, suffix) {
                    return str.indexOf(suffix, str.length - str.length) !== -1;
                }
                
                source: endsWith(modelData.image, ".qml") ? modelData.image : "qrc:/qml/FeaturedImage.qml"
                width: info.width
                height: info.height
            }
        }
    Item {
        id: titleBar
        height: 40
        z: 23
        property variant modelData: info.model.get(Math.min(info.currentIndex, info.model.count))
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
            iconName: "go-previous"
            height: parent.height*0.9
            width: height
            onClicked: info.previous()
            anchors {
                verticalCenter: parent.verticalCenter
                left: parent.left
                leftMargin: 3
            }
        }
        
        Image {
            id: iconItem
            anchors {
                left: prevButton.right
                top: parent.top
                bottom: parent.bottom
                margins: 3
            }
            width: height
            source: titleBar.modelData ? "image://icon/"+titleBar.modelData.icon : "image://icon/kde"
            sourceSize.width: width
            sourceSize.height: height
        }
        
        Label {
            anchors {
                left: iconItem.right
                verticalCenter: parent.verticalCenter
                leftMargin: 10
            }
            text: titleBar.modelData ? i18n("<b>%1</b><br/>%2", titleBar.modelData.text, titleBar.modelData.comment) : ""
        }
        ToolButton {
            iconName: "go-next"
            height: parent.height*0.9
            width: height
            onClicked: info.next()
            anchors {
                right: parent.right
                verticalCenter: parent.verticalCenter
                rightMargin: 3
            }
        }
    }
}
