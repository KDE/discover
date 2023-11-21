import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.discover as Discover

ConditionalLoader {
    id: root

    property alias application: listener.resource

    readonly property alias isActive: listener.isActive
    readonly property bool isStateAvailable: application.state !== Discover.AbstractResource.Broken
    readonly property alias listener: listener
    property bool availableFromOnlySingleSource: false

    Discover.TransactionListener {
        id: listener
    }

    readonly property Kirigami.Action action: Kirigami.Action {
        text: {
            if (!root.isStateAvailable) {
                return i18nc("State being fetched", "Loading…")
            }
            if (!application.isInstalled) {
                if (root.availableFromOnlySingleSource) {
                    return i18nc("@action:button %1 is the name of a software repository", "Install from %1", application.displayOrigin);
                }
                return i18nc("@action:button", "Install");
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
                Discover.ResourcesModel.removeApplication(application);
            } else {
                Discover.ResourcesModel.installApplication(application);
            }
        } else {
            console.warn("trying to un/install but resource still active", application.name);
        }
    }

    condition: listener.isActive
    componentTrue: RowLayout {
        QQC2.ToolButton {
            Layout.fillHeight: true
            action: root.cancelAction

            display: QQC2.AbstractButton.IconOnly

            QQC2.ToolTip.text: text
            QQC2.ToolTip.visible: hovered
            QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
        }

        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress / 100
        }
    }

    componentFalse: QQC2.Button {
        visible: !application.isInstalled || application.isRemovable
        enabled: application.state !== Discover.AbstractResource.Broken
        activeFocusOnTab: false

        text: root.action.text
        icon.name: root.action.icon.name
        display: QQC2.AbstractButton.IconOnly

        QQC2.ToolTip.text: text
        QQC2.ToolTip.visible: hovered
        QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay

        onClicked: root.click()
    }
}
