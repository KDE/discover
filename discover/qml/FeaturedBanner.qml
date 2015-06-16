import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0
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

    Rectangle {
        anchors.fill: titleBar
        color: palette.midlight
        opacity: 0.7
        z: 20
    }

    SystemPalette { id: palette }

    RowLayout {
        id: titleBar
        height: description.paintedHeight*1.2
        z: 23
        spacing: 10
        property variant modelData: info.model.get(Math.min(info.currentIndex, info.model.count))
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        
        ToolButton {
            iconName: "go-previous"
            Layout.fillHeight: true
            width: height
            onClicked: info.previous()
        }
        
        QIconItem {
            height: parent.height*2
            width: parent.height*2
            icon: titleBar.modelData ? titleBar.modelData.icon : "kde"
        }
        
        Label {
            id: description
            Layout.fillWidth: true
            anchors.verticalCenter: parent.verticalCenter

            text: titleBar.modelData ? i18n("<b>%1</b><br/>%2", titleBar.modelData.text, titleBar.modelData.comment) : ""
        }
        ToolButton {
            iconName: "go-next"
            Layout.fillHeight: true

            width: height
            onClicked: info.next()
        }
    }
}
