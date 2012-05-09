import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import "navigation.js" as Navigation
import "Helpers.js" as Helpers

Item {
    property QtObject category
    id: topItem
    
    onCategoryChanged: {
        categoryModel.insert(0, {
                            "text": topItem.category.name,
                            "color": "transparent",
                            "icon": topItem.category.icon,
                            "packageName": "" })
    }
    
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
            if(category)
                return
            
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
            ListElement { text: "KDE Workspace"; color: "#3333ff"; icon: "kde"; packageName: "" }
        }
        
        ListModel { id: categoryModel }
        
        dataModel: category==null ? noCategoryModel : categoryModel
        
        delegate: MouseArea {
                property QtObject modelData
                id: infoArea
                enabled: modelData.package!=""
                anchors.fill: parent
                onClicked: Navigation.openApplication(app.appBackend.applicationByPackageName(modelData.packageName))
                
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: modelData.color
                    opacity: 0.3
                    
                    Behavior on opacity { NumberAnimation { duration: 500 } }
                }
                
                Label {
                    anchors.margins: 20
                    anchors.left: iconItem.right
                    anchors.verticalCenter: parent.verticalCenter
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    text: modelData.text
                    font.pointSize: info.height/3
                }
                
                QIconItem {
                    id: iconItem
                    anchors {
                        margins: 5
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: height
                    icon: modelData.icon
                }
        }
    }
}