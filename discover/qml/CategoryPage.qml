import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Page {
    id: page
    property QtObject category
    
    
    function searchFor(text) {
        if(category)
            Navigation.openApplicationList(category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList("edit-find", i18n("Search..."), category, text)
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

    property int minCellWidth: 130
    
    GridView {
        id: view
        cellWidth: view.width/Math.floor(view.width/minCellWidth)-1
        cellHeight: 100
        
        anchors {
            top: parent.top
            left: parent.left
            right: scroll.left
            bottom: parent.bottom
            leftMargin: scroll.width
        }
        model: cats
        clip: true
        delegate: categoryDelegate
        header: CategoryHeader {
            anchors.leftMargin: scroll.width
            category: page.category
            width: parent.width-scroll.width
            height: 100
        }
        footer: topsView
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
    Component {
        id: topsView
        Item {
            height: Math.min(200, page.height/2)
            width: view.width-scroll.width
            ListView {
                id: top1
                interactive: false
                anchors {
                    top: parent.top
                    left: parent.left
                    bottom: parent.bottom
                }
                width: parent.width/2-10
                header: Label { text: i18n("<b>Popularity Contest</b>"); width: top1.width; horizontalAlignment: Text.AlignHCenter }
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
                            MouseArea { anchors.fill: parent; onClicked: Navigation.openApplication(application) }
                        }
            }
            ListView {
                id: top2
                interactive: false
                height: parent.height
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                }
                width: parent.width/2-10
                header: Label { text: i18n("<b>Best Ratings</b>"); width: top2.width; horizontalAlignment: Text.AlignHCenter }
                model: ApplicationProxyModel {
                    id: ratingsTopModel
                    filteredCategory: page.category
                    stringSortRole: "ratingPoints"
                    sortOrder: Qt.DescendingOrder
                }
                Connections {
                    ignoreUnknownSignals: true
                    target: app.appBackend ? app.appBackend.reviewsBackend() : null
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
                            MouseArea { anchors.fill: parent; onClicked: Navigation.openApplication(application) }
                        }
            }
        }
    }
}
    