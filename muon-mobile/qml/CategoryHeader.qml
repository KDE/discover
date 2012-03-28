import QtQuick 1.0
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1

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
        width: parent.width-10
        
        ListModel {
            id: noCategoryModel
            ListElement { text: "KDE Workspace"; color: "red"; icon: "kde"; packageName: "" }
            ListElement { text: "KAlgebra"; color: "#cc77cc"; icon: "kalgebra"; packageName: "kalgebra" }
            ListElement { text: "Digikam"; color: "#9999ff"; icon: "digikam"; packageName: "digikam" }
            ListElement { text: "Plasma"; color: "#bd9"; icon: "plasma"; packageName: "" }
        }
        
        ListModel { id: categoryModel }
        
        dataModel: category==null ? noCategoryModel : categoryModel
        
        delegate: Item {
                property QtObject modelData
                
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: modelData.color
                    opacity: 0.5
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
                        margins: 20
                        left: parent.left
                        top: parent.top
                        bottom: parent.bottom
                    }
                    width: height
                    icon: modelData.icon
                }
                
                MouseArea {
                    enabled: modelData.packageName!=""
                    anchors.fill: parent
                    onClicked: openApplication(modelData.packageName)
                }
        }
    }
}