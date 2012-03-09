import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Page {
    property QtObject category
    
    function searchFor(text) {
        console.log("search!! "+text)
        if(category)
            Navigation.openApplicationList(category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList("go-home", i18n("Search..."), category, text)
    }

    Component {
        id: categoryDelegate
        ListItem {
            width: view.cellWidth -10
            height: view.cellHeight -10
            Column {
                anchors.fill: parent
                spacing: 10
                QIconItem {
                    icon: decoration
                    width: 40; height: 40
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Label {
                    text: display
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                
            }
            Rectangle {
                anchors.fill: parent
                color: "white"
                opacity: itemArea.containsMouse ? 0.3 : 0 
            }
            MouseArea {
                id: itemArea
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    var cat = cats.categoryForIndex(index)
                    switch(categoryType) {
                        case CategoryModel.CategoryType:
                            Navigation.openApplicationList(category.icon, category.name, cat, "")
                            break;
                        case CategoryModel.SubCatType:
                            Navigation.openCategory(category.icon, category.name, cat)
                            break;
                    }
                }
            }
        }
    }

    GridView {
        cellWidth: 130
        cellHeight: 100
        
        anchors {
            top: parent.top
            left: parent.left
            bottom: parent.bottom
            right: scroll.left
        }
        id: view
        model: cats
        anchors.fill: parent
        delegate: categoryDelegate
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
                top: parent.top
                right: parent.right
                bottom: parent.bottom
        }
    }
    
    CategoryModel {
        id: cats
        Component.onCompleted: {
            if(category)
                setSubcategories(category)
            else
                populateCategories(i18n("Get Software"))
        }
    }
}
    