import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

ListView
{
    property alias application: model.application
    model: ApplicationAddonsModel { id: model }
    
    anchors.leftMargin: scroll.width
    delegate: Row {
        height: 50
        width: 50
        spacing: 10
        CheckBox {
            anchors.verticalCenter: parent.verticalCenter
            checked: model.checked
        }
        Image {
            source: "image://icon/applications-other"
            height: parent.height; width: height
            smooth: true
        }
        Label {
            anchors.verticalCenter: parent.verticalCenter
            text: i18n("<qt>%1<br/><em>%2</em></qt>", display, toolTip)
        }
    }
    
    ScrollBar {
        id: scroll
        orientation: Qt.Vertical
        flickableItem: parent
        stepSize: 40
        scrollButtonInterval: 50
        anchors {
            top: parent.top
            right: parent.right
            bottom: parent.bottom
        }
    }
    clip: true
}