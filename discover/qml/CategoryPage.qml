import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0
import "navigation.js" as Navigation

Page {
    id: page
    property QtObject category
    property real actualWidth: width-Math.pow(width/70, 2)
    
    function searchFor(text) {
        if(category)
            Navigation.openApplicationList(category.icon, i18n("Search in '%1'...", category.name), category, text)
        else
            Navigation.openApplicationList("edit-find", i18n("Search..."), category, text)
    }
    
    Component {
        id: categoryDelegate
        ListItem {
            property int minCellWidth: 130
            width: parent.width/Math.ceil(parent.width/minCellWidth)-10
            height: 100
            enabled: true
            Column {
                anchors.centerIn: parent
                width: parent.width
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
            onClicked: {
                switch(categoryType) {
                    case CategoryModel.CategoryType:
                        Navigation.openApplicationList(category.icon, category.name, category, "")
                        break;
                    case CategoryModel.SubCatType:
                        Navigation.openCategory(category)
                        break;
                }
            }
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: flick
        anchors {
            top: flick.top
            right: parent.right
            bottom: flick.bottom
        }
    }

    Flickable {
        id: flick
        anchors {
            horizontalCenter: parent.horizontalCenter
            top: parent.top
            bottom: parent.bottom
            bottomMargin: 10
        }
        width: page.actualWidth
        contentHeight: conts.height
        
        Column {
            id: conts
            width: parent.width
            spacing: 10
            Loader {
                width: flick.width
                Component {
                    id: categoryHeader
                    CategoryHeader {
                        category: page.category
                        height: 128
                    }
                }
                
                Component {
                    id: featured
                    FeaturedBanner {
                        height: 310
                        clip: true
                    }
                }
                sourceComponent: category==null ? featured : categoryHeader
            }
            
            Flow {
                width: parent.width
                spacing: 10
                Repeater {
                    model: CategoryModel {
                        displayedCategory: page.category
                    }
                    delegate: categoryDelegate
                }
            }
            
            Item {
                height: Math.min(200, page.height/2)
                width: parent.width
                ApplicationsTop {
                    id: top1
                    width: parent.width/2-5
                    anchors {
                        top: parent.top
                        left: parent.left
                        bottom: parent.bottom
                    }
                    sortRole: "popcon"
                    filteredCategory: page.category
                    header: Label { text: i18n("<b>Popularity Contest</b>"); width: top1.width; horizontalAlignment: Text.AlignHCenter }
                    roleDelegate: Label { property variant model: null; text: i18n("points: %1", model.popcon) }
                    Component.onCompleted: top1.sortModel()
                }
                ApplicationsTop {
                    id: top2
                    interactive: false
                    anchors {
                        top: parent.top
                        right: parent.right
                        bottom: parent.bottom
                    }
                    width: parent.width/2-5
                    sortRole: "ratingPoints"
                    filteredCategory: page.category
                    header: Label { text: i18n("<b>Best Ratings</b>"); width: top2.width; horizontalAlignment: Text.AlignHCenter }
                    roleDelegate: Rating { property variant model: null; rating: model.rating; height: 10 }
                }
            }
        }
    }
}
