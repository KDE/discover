import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Page {
    id: page
    property QtObject category
    
    
    function searchFor(text) {
        console.log("search!! "+text)
        if(category)
            Navigation.openApplicationList(pageStack, category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList(pageStack, "edit-find", i18n("Search..."), category, text)
    }
    
    function openApplication(packageName) {
        var application = app.appBackend.applicationByPackageName(packageName)
        console.log("opening "+packageName + "..."+application)
        Navigation.openApplication(pageStack, application)
    }
    
    tools: TextField {
        id: searchInput
        width: 80
        placeholderText: i18n("Search... ")
        onTextChanged: searchFor(text)
        opacity: page.status == PageStatus.Active ? 1 : 0
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
                    anchors {
                        left: parent.left
                        right: parent.right
                    }
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
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
                            Navigation.openApplicationList(pageStack, category.icon, category.name, cat, "")
                            break;
                        case CategoryModel.SubCatType:
                            Navigation.openCategory(pageStack, category.icon, category.name, cat)
                            break;
                    }
                }
            }
        }
    }

    property int minCellWidth: 130
    
    GridView {
        id: view
        cellWidth: view.width/Math.floor(view.width/minCellWidth)-1
        cellHeight: 100
        
        anchors {
            top: parent.top
            left: parent.left
            right: scroll.left
            bottom: top1.top
        }
        model: cats
        clip: true
        delegate: categoryDelegate
        header: CategoryHeader {
            category: page.category
            width: parent.width
            height: 100
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: view
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
                top: view.top
                right: parent.right
                bottom: view.bottom
        }
    }
    
    CategoryModel {
        id: cats
        displayedCategory: page.category
    }
    
    ListView {
        id: top1
        clip: true
        interactive: false
        anchors {
            top: top2.top
            left: parent.left
            bottom: parent.bottom
        }
        width: parent.width/2-10
        header: Label { text: i18n("<b>Popularity Contest</b>") }
        model: ApplicationProxyModel {
            stringSortRole: "popcon"
            sortOrder: Qt.DescendingOrder
            filteredCategory: page.category
            
            Component.onCompleted: sortModel()
        }
        delegate: ListItem {
                    width: top1.width
                    height: 30
                    QIconItem {
                        id: iconItem
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        height: parent.height*0.9
                        width: height
                        icon: model["icon"]
                    }
                    Label {
                        anchors { left: iconItem.right; right: pointsLabel.left; verticalCenter: parent.verticalCenter }
                        text: name
                        elide: Text.ElideRight
                    }
                    Label {
                        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                        id: pointsLabel
                        text: i18n("points: %1", popcon)
                    }
                    MouseArea { anchors.fill: parent; onClicked: Navigation.openApplication(pageStack, application) }
                }
    }
    ListView {
        id: top2
        clip: true
        interactive: false
        height: Math.min(200, parent.height/2)
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
        width: parent.width/2-10
        header: Label { text: i18n("<b>Best Ratings</b>") }
        model: ApplicationProxyModel {
            id: ratingsTopModel
            filteredCategory: page.category
            stringSortRole: "ratingPoints"
            sortOrder: Qt.DescendingOrder
        }
        Connections {
            ignoreUnknownSignals: true
            target: app.appBackend.reviewsBackend()
            onRatingsReady: ratingsTopModel.sortModel()
        }
        delegate: ListItem {
                    width: top1.width
                    height: 30
                    QIconItem {
                        id: iconItem
                        anchors { left: parent.left; verticalCenter: parent.verticalCenter }
                        height: parent.height*0.9
                        width: height
                        icon: model["icon"]
                    }
                    Label {
                        anchors { left: iconItem.right; right: ratingsItem.left; verticalCenter: parent.verticalCenter }
                        text: name
                        elide: Text.ElideRight
                    }
                    Rating {
                        id: ratingsItem
                        anchors { verticalCenter: parent.verticalCenter; right: parent.right }
                        rating: model.rating
                        height: 10
                    }
                    MouseArea { anchors.fill: parent; onClicked: Navigation.openApplication(pageStack, application) }
                }
    }
}
    