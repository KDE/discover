import QtQuick 2.1
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

ConditionalLoader {
    id: root

    property Component additionalItem: null
    property alias application: listener.resource

    readonly property alias isActive: listener.isActive
    readonly property alias progress: listener.progress
    readonly property bool isStateAvailable: application.state !== AbstractResource.Broken
    readonly property alias listener: listener

    TransactionListener {
        id: listener
    }

    readonly property Kirigami.Action action: Kirigami.Action {
        text: {
            if (!root.isStateAvailable) {
                return i18nc("State being fetched", "Loading…")
            }
            if (!application.isInstalled) {
                return i18n("Install");
            }
            return i18n("Remove");
        }
        icon {
            name: application.isInstalled ? "edit-delete" : "download"
            color: !listener.isActive && enabled
                ? (application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor)
                : Kirigami.Theme.backgroundColor
        }
        visible: !listener.isActive && (!application.isInstalled || application.isRemovable)
        enabled: !listener.isActive && root.isStateAvailable
        onTriggered: root.click()
    }
    readonly property Kirigami.Action cancelAction: Kirigami.Action {
        text: i18n("Cancel")
        icon.name: "dialog-cancel"
        enabled: listener.isCancellable
        tooltip: listener.statusText
        onTriggered: {
            listener.cancel()
            enabled = false
        }
        visible: listener.isActive
        onVisibleChanged: enabled = true
    }

    function click() {
        if (!isActive) {
            if(application.isInstalled) {
                ResourcesModel.removeApplication(application);
            } else {
                ResourcesModel.installApplication(application);
            }
        } else {
            console.warn("trying to un/install but resource still active", application.name);
        }
    }

    condition: listener.isActive
    componentTrue: RowLayout {
        ToolButton {
            Layout.fillHeight: true
            action: root.cancelAction
            text: ""
            ToolTip.visible: hovered
            ToolTip.text: root.cancelAction.text
        }

        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress / 100
        }
    }

    componentFalse: Button {
        visible: !application.isInstalled || application.isRemovable
        enabled: application.state !== AbstractResource.Broken
        text: root.action.text

        activeFocusOnTab: false
        onClicked: root.click()
    }
}
