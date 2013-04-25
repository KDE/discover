import QtQuick 1.0
import org.kde.plasma.core 0.1
import org.kde.plasma.components 0.1
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
                    Navigation.openApplication(resourcesModel.resourceByPackageName(modelData.packageName))
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
                    height: parent.height*0.9
                    width: height
                    onClicked: info.previous()
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: 3
                    }
                }
                
                IconItem {
                    id: iconItem
                    anchors {
                        left: prevButton.right
                        top: parent.top
                        bottom: parent.bottom
                        margins: 3
                    }
                    width: height
                    source: modelData.icon
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
}
