import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.kirigami 2.0 as Kirigami

ConditionalLoader
{
    id: root
    property alias application: listener.resource
    readonly property alias isActive: listener.isActive
    readonly property alias progress: listener.progress
    readonly property alias listener: listener
    readonly property string text: !application.isInstalled ? i18n("Install") : i18n("Remove")
    property Component additionalItem: null

    TransactionListener {
        id: listener
    }

    property QtObject action: Kirigami.Action {
        text: root.text
        icon {
            name: application.isInstalled ? "trash-empty" : "cloud-download"
            color: application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor
        }
        visible: !listener.isActive && !applicationWindow().wideScreen
        onTriggered: root.click()
    }

    function click() {
        if (!isActive) {
            if(application.isInstalled)
                ResourcesModel.removeApplication(application);
            else
                ResourcesModel.installApplication(application);
        } else {
            console.warn("trying to un/install but resouce still active", application.name)
        }
    }

    condition: listener.isActive
    componentTrue: RowLayout {
        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress/100
        }

        ToolButton {
            Layout.fillHeight: true
            iconName: "dialog-cancel"
            enabled: listener.isCancellable
            onClicked: listener.cancel()
        }
    }

    componentFalse: Button {
        enabled: application.state != AbstractResource.Broken
        text: root.text
        focus: true

        onClicked: root.click()
    }
}
