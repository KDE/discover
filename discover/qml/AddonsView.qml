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
            height: 50
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
            iconSource: "dialog-ok"
            text: i18n("Install")
            onClicked: addonsModel.applyChanges()
        }
        Button {
            iconSource: "document-revert"
            text: i18n("Discard")
            onClicked: addonsModel.discardChanges()
        }
    }
}
