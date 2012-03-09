import QtQuick 1.1
import org.kde.plasma.components 0.1

CommonDialog {
    property QtObject application
    property alias rating: ratingInput.rating
    property alias summary: summaryInput.text
    property alias review: reviewInput.text
    id: reviewDialog
    onClickedOutside: reviewDialog.close()
    titleText: i18n("Reviewing %1", application.name)
    buttons: Button { text: i18n("Submit"); onClicked: reviewDialog.accept() }
    
    content: Item {
        height: 200
        width: 200
        
        Column {
            id: info
            spacing: 5
            anchors.left: parent.left
            anchors.right: parent.right
            Label { text: i18n("Rating:") }
            Rating {
                id: ratingInput
                editable: true
            }
            
            Label { text: i18n("Summary:") }
            TextField {
                id: summaryInput
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }
        
        TextArea {
            id: reviewInput
            width: parent.width
            anchors.top: info.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 5
        }
    }
}