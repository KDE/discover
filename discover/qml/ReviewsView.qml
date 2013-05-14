import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.plasma.extras 0.1
import org.kde.muon 1.0

Item
{
    property alias application: reviewsModel.resource
    property bool hasReviews: reviewsView.count>0
    
    ListView {
        id: reviewsView
        clip: true
        anchors {
            fill: parent
            rightMargin: scroll.width
        }
        visible: reviewsView.count>0
        spacing: 5
        
        header: Heading { text: i18n("<b>Reviews:</b>") }
        
        delegate: ListItem {
            visible: model["shouldShow"]
            height: content.height+10
            
            function usefulnessToString(favorable, total)
            {
                return total==0
                        ? i18n("<em>Tell us about this review!</em>")
                        : i18n("<em>%1 out of %2 people found this review useful</em>", favorable, total)
            }
            MouseArea {
                id: delegateArea
                hoverEnabled: true
                width: parent.width
                height: content.height
            
                Label {
                    anchors {
                        left: parent.left
                        right: rating.left
                    }
                    
                    id: content
                    text: i18n("<p style='margin: 0 0 0 0'><b>%1</b> by %2</p><p style='margin: 0 0 0 0'>%3</p><p style='margin: 0 0 0 0'>%4</p>", summary, reviewer,
                            display, usefulnessToString(usefulnessFavorable, usefulnessTotal))
                    wrapMode: Text.WordWrap
                }
                
                Label {
                    anchors {
                        right: parent.right
                        bottom: parent.bottom
                        bottomMargin: -5
                    }
                    opacity: delegateArea.containsMouse ? 1 : 0.2
                    
                    text: {
                        if (usefulChoice == ReviewsModel.Yes) {
                            i18n("<em>Useful? <a href='true'><b>Yes</b></a>/<a href='false'>No</a></em>")
                        } else if (usefulChoice == ReviewsModel.No) {
                            i18n("<em>Useful? <a href='true'>Yes</a>/<a href='false'><b>No</b></a></em>")
                        } else {
                            i18n("<em>Useful? <a href='true'>Yes</a>/<a href='false'>No</a></em>")
                        }
                    }
                    onLinkActivated: reviewsModel.markUseful(index, link=='true')
                }
                
                Rating {
                    id: rating
                    anchors.top: parent.top
                    anchors.right: parent.right
                    rating: model["rating"]
                    height: content.font.pixelSize
                    layoutDirection: Qt.RightToLeft
                }
            }
        }
        
        model: ReviewsModel {
            id: reviewsModel
        }
    }
    
    NativeScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: reviewsView
        anchors {
            top: reviewsView.top
            right: parent.right
            bottom: reviewsView.bottom
        }
    }
}
