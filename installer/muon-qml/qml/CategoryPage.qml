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
            Row {
                spacing: 10
                QIconItem { icon: decoration; width: 40; height: 40 }
                Label { text: display }
            }
            MouseArea {
                anchors.fill: parent
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

    ListView {
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
    