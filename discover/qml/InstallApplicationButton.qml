import QtQuick 2.1
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.kirigami 2.14 as Kirigami

ConditionalLoader
{
    id: root
    property Component additionalItem: null
    property bool compact: false
    property bool appIsFromNonDefaultBackend: false
    property string backendName: ""

    property alias application: listener.resource

    readonly property alias isActive: listener.isActive
    readonly property alias progress: listener.progress
    readonly property bool isStateAvailable: application.state !== AbstractResource.Broken
    readonly property alias listener: listener
    readonly property string text: {
        if (!root.isStateAvailable) {
            return i18nc("State being fetched", "Loading…")
        }
        if (!application.isInstalled) {
            // Must be from a non-default backend; tell the user where it's from
            if (backendName.length !== 0) {
                return i18nc("Install the version of an app that comes from Snap, Flatpak, etc", "Install from %1", backendName);
            }
            return i18n("Install");
        }
        return i18n("Remove");
    }

    TransactionListener {
        id: listener
    }

    readonly property Kirigami.Action action: Kirigami.Action {
        text: root.text
        icon {
            name: application.isInstalled ? "edit-delete" : "download"
            color: !enabled ? Kirigami.Theme.backgroundColor : !listener.isActive ? (application.isInstalled ? Kirigami.Theme.negativeTextColor : Kirigami.Theme.positiveTextColor) : Kirigami.Theme.backgroundColor
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
            text: ""
            ToolTip.visible: hovered
            ToolTip.text: root.cancelAction.text
        }

        LabelBackground {
            Layout.fillWidth: true
            text: listener.statusText
            progress: listener.progress/100
        }
    }

    componentFalse: Button {
        enabled: application.state !== AbstractResource.Broken
        // This uses a custom content item to be able to use an icon that's
        // loaded from a remote location, which does not work with the standard
        // button. This works around https://bugs.kde.org/show_bug.cgi?id=433433
        // TODO: just set the button's icon.source property when that's fixed
        implicitWidth: overrideLayout.implicitWidth
        contentItem: RowLayout {
            id: overrideLayout
            Item { // Left padding
                implicitWidth: Kirigami.Units.smallSpacing
            }
            Kirigami.Icon {
                visible: source != ""
                source: compact ? root.action.icon.name : (root.appIsFromNonDefaultBackend ? application.sourceIcon : "")
                implicitWidth: Kirigami.Units.iconSizes.smallMedium
                implicitHeight: Kirigami.Units.iconSizes.smallMedium
                smooth: !compact
            }
            Label {
                visible: !compact
                text: compact ? "" : root.text
            }
            Item { // right padding
                implicitWidth: Kirigami.Units.smallSpacing
            }
        }
        activeFocusOnTab: false
        onClicked: root.click()
    }
}
