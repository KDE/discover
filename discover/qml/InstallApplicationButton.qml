import QtQuick 2.1
import QtQuick.Controls 2.3
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

    readonly property Kirigami.Action action: Kirigami.Action {
        text: root.text
        icon {
            name: application.isInstalled ? "trash-empty" : "cloud-download"
            color: !enabled ? Kirigami.Theme.backgroundColor : !listener.isActive ? (application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor) : Kirigami.Theme.backgroundColor
        }
        enabled: !listener.isActive && application.state != AbstractResource.Broken
        onTriggered: root.click()
    }
    readonly property Kirigami.Action cancelAction: Kirigami.Action {
        text: i18n("Cancel")
        icon.name: "dialog-cancel"
        enabled: listener.isCancellable
        onTriggered: listener.cancel()
        visible: listener.isActive
    }

    function click() {
        if (!isActive) {
            if(application.isInstalled)
                ResourcesModel.removeApplication(application);
            else
                ResourcesModel.installApplication(application);
        } else {
            console.warn("trying to un/install but resource still active", application.name)
        }
    }

    condition: listener.isActive
    componentTrue: RowLayout {
        ToolButton {
            Layout.fillHeight: true
            action: root.cancelAction
        }

        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress/100
        }
    }

    componentFalse: Button {
        enabled: application.state != AbstractResource.Broken
        text: root.text
        focus: true

        onClicked: root.click()
    }
}
