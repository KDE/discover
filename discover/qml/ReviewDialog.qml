import QtQuick 2.1
import org.kde.plasma.components 2.0

CommonDialog {
    property QtObject application
    property alias rating: ratingInput.rating
    property alias summary: summaryInput.text
    property alias review: reviewInput.text
    id: reviewDialog
    onClickedOutside: reviewDialog.close()
    titleText: i18n("Reviewing %1", application.name)
    buttons: Item {
        height: 30
        width: 200
        Button {
            id: submitButton
            height: parent.height
            anchors.left: parent.left
            anchors.margins: 10
            width: 100
            text: i18n("Submit"); onClicked: reviewDialog.accept()
            iconSource: "dialog-accept"
        }
        Button {
            height: parent.height
            width: 100
            anchors.margins: 10
            anchors.right: parent.right; anchors.left: submitButton.right
            iconSource: "dialog-close"
            text: i18n("Close"); onClicked: reviewDialog.reject()
        }
    }
    
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
