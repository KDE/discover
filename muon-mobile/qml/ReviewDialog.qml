import QtQuick 1.1
import org.kde.plasma.components 0.1

Dialog {
    id: reviewDialog
    onAccepted: console.log("send!")
    onClickedOutside: reviewDialog.close()
    
    buttons: Button { text: i18n("Submit"); onClicked: reviewDialog.accept() }
    
    content: Item {
        height: 200
        width: 200
        
        Column {
            id: info
            spacing: 10
            Row { Label { text: "rate!" } Rating {} }
            Row { Label { text: i18n("Summary:") } TextField {} }
        }
        
        TextArea {
            width: parent.width
            anchors.top: info.bottom
            anchors.bottom: parent.bottom
        }
    }
}