import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.qtextracomponents 0.1
import org.kde.muon 1.0

Page
{
    property QtObject application
    anchors.margins: 20
    
    Row {
        id: header
        spacing: 20
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        QIconItem {
            id: icon
            width: 40
            height: 40
            
            icon: application.icon
        }
        
        Column {
            Label {
                font.bold: true
                text: application.name
            }
            Label { text: application.comment }
        }
        Column {
            Label { text: "rating..." }
            Label { text: "N reviews" }
        }
        
        Image {
            id: screenshot
            width: 100
            height: 100
            
            source: application.screenshotUrl()
        }
    }
    
    Column {
        anchors.top: header.bottom
        Label { text: "Description\nLink" }
        
        Label {
            text: i18n(  "<b>Total Size:</b> %1 to download, %2 on disk.<br/>"
                        +"<b>Version:</b> %3<br/>"
                        +"<b>License:</b> %4<br/>"
                        +"<b>Support:</b> %5<br/>", 1,2,3,4,5
            )
        }
        
        Label { text: "Reviews" }
    }
}