import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page {
    property variant category
    
    function searchFor(text) {
        console.log("search!! "+text)
        openApplicationList(category, text)
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
                            openApplicationList(cat, "")
                            break;
                        case CategoryModel.SubCatType:
                            openCategory(cat)
                            break;
                    }
                }
            }
        }
    }

    ListView {
        model: cats
        anchors.fill: parent
        delegate: categoryDelegate
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
    