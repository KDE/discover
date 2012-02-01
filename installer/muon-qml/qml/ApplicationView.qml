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
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 10
        
        Button {
            text: application.isInstalled ? i18n("Remove") : i18n("Install")
            onClicked: console.log("install "+application.name)
        }
        
        Label {
            width: parent.width
            wrapMode: Text.WordWrap
            text: i18n("<b>Description:</b><br/>%1", application.longDescription)
        }
        
        Button {
            text: i18n("Homepage")
            enabled: application.homepage
            onClicked: console.log("open "+application.homepage)
        }
        
        Label {
            text: i18n(  "<b>Total Size:</b> %1<br/>"
                        +"<b>Version:</b> %2<br/>"
                        +"<b>License:</b> %3<br/>",
                         application.sizeDescription,
                         application.name+" "+(application.isInstalled ?
                                                    application.installedVersion : application.availableVersion),
                         application.license
            )
        }
        
        Label { text: "Reviews" }
    }
}