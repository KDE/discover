import QtQuick 1.1
import org.kde.plasma.components 0.1
import org.kde.muon 1.0

Column
{
    id: addonsView
    property alias application: addonsModel.application
    property bool isInstalling: false
    property alias isEmpty: addonsModel.isEmpty

    Repeater
    {
        model: ApplicationAddonsModel { id: addonsModel }
        
        delegate: Row {
            height: description.paintedHeight*1.2
            width: 50
            spacing: 10
            CheckBox {
                enabled: !addonsView.isInstalling
                anchors.verticalCenter: parent.verticalCenter
                checked: model.checked
                onClicked: addonsModel.changeState(display, checked)
            }
            Image {
                source: "image://icon/applications-other"
                height: parent.height; width: height
                smooth: true
                opacity: addonsView.isInstalling ? 0.3 : 1
            }
            Label {
                id: description
                enabled: !addonsView.isInstalling
                anchors.verticalCenter: parent.verticalCenter
                text: i18n("<qt>%1<br/><em>%2</em></qt>", display, toolTip)
            }
        }
    }
    
    Row {
        enabled: addonsModel.hasChanges && !addonsView.isInstalling
        spacing: 5
        
        Button {
            height: parent.enabled ? implicitHeight : 0
            visible: height!=0
            iconSource: "dialog-ok"
            text: i18n("Install")
            onClicked: addonsModel.applyChanges()
            Behavior on height { NumberAnimation { duration: 100 } }
        }
        Button {
            height: parent.enabled ? implicitHeight : 0
            visible: height!=0
            iconSource: "document-revert"
            text: i18n("Discard")
            onClicked: addonsModel.discardChanges()
            Behavior on height { NumberAnimation { duration: 100 } }
        }
    }
}
