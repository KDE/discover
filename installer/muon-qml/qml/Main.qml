import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Rectangle {
    height: 400
    width: 300
    color: "lightgrey"
    
    Component {
        id: categoryComp
        
        Page {
            property variant category
            property QtObject model: cats
            
            ListView {
                model: cats
                anchors.fill: parent
                delegate: categoryDelegate
            }
            
            CategoryModel {
                id: cats
                Component.onCompleted: {
                    console.log("lalala "+category)
                    if(category)
                        setSubcategories(category)
                    else
                        populateCategories(i18n("Get Software"))
                }
            }
        }
    }
    
    Component {
        id: categoryDelegate
        ListItem {
            Row {
                spacing: 10
                QIconItem { icon: decoration; width: 40; height: 40 }
                Label { text: i18n("%1", display) }
            }
            MouseArea { anchors.fill: parent; onClicked: goToPage(index) }
        }
    }
    
    function goToPage(idx) {
        try {
            console.log("......... "+pageStack.currentPage)
            var cat = pageStack.currentPage.model.categoryForIndex(idx)
            var obj = categoryComp.createObject(pageStack, { category: cat })
            pageStack.push(obj);
        } catch (e) {
            console.log("error: "+e)
        }
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.fill: parent
    }
    
    Component.onCompleted: {
        pageStack.initialPage = categoryComp.createObject(pageStack)
    }
}