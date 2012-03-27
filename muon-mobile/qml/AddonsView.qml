import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

ListView
{
    id: addonsView
    property alias application: addonsModel.application
    property alias addonsHaveChanged: addonsModel.hasChanges
    property bool isInstalling: false
    property bool isEmpty: addonsView.count == 0
    model: ApplicationAddonsModel { id: addonsModel }
    
    delegate: Row {
        height: 50
        width: 50
        spacing: 10
        CheckBox {
            enabled: !isInstalling
            anchors.verticalCenter: parent.verticalCenter
            checked: model.checked
            onClicked: addonsModel.changeState(display, checked)
        }
        Image {
            source: "image://icon/applications-other"
            height: parent.height; width: height
            smooth: true
            opacity: isInstalling ? 0.3 : 1
        }
        Label {
            enabled: !isInstalling
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
    
    header: Row {
        id: buttonsRow
        layoutDirection: Qt.RightToLeft
        anchors {
            top: parent.top
            right: parent.right
        }
        spacing: 5
        
        Button {
            iconSource: "dialog-ok"
            text: i18n("Apply")
            onClicked: addonsModel.applyChanges()
            enabled: addonsModel.hasChanges && !isInstalling
        }
        Button {
            iconSource: "document-revert"
            text: i18n("Discard")
            enabled: addonsModel.hasChanges && !isInstalling
            onClicked: addonsModel.discardChanges()
        }
    }
    clip: true
}