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
    // Arbitrary "very long" limit of 15 characters; any longer than this and
    // it's not a good idea to show the whole thing in the button or else it
    // will elide the app name and may even overflow the layout!
    readonly property bool backendNameIsVeryLong: backendName.length !== 0 && backendName.length > 15
    readonly property string text: {
        if (!root.isStateAvailable) {
            return i18nc("State being fetched", "Loading…")
        }
        if (!application.isInstalled) {
            // Must be from a non-default backend; tell the user where it's from
            if (backendName.length !== 0 && !backendNameIsVeryLong) {
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
        visible: !application.isInstalled || application.isRemovable
        enabled: application.state !== AbstractResource.Broken
        // This uses a custom content item to be able to use an icon that's
        // loaded from a remote location, which does not work with the standard
        // button. This works around https://bugs.kde.org/show_bug.cgi?id=433433
        // TODO: just set the button's icon.source property when that's fixed
        rightPadding: Kirigami.Units.smallSpacing
        leftPadding: Kirigami.Units.smallSpacing

        // Remove when we depend on KF 5.87 (needs edc9094313e24d044c5fba2b2cfefc7db12fb187 qqc2-desktop-style)
        implicitWidth: Math.max(implicitBackgroundWidth  + leftInset + rightInset,
                                implicitContentWidth + leftPadding + rightPadding)

        contentItem: RowLayout {
            Item { // Left padding
                Layout.fillWidth: true
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
                Layout.fillWidth: true
            }
        }
        activeFocusOnTab: false
        onClicked: root.click()

        ToolTip.visible: hovered && !application.isInstalled && root.backendNameIsVeryLong
        ToolTip.text: backendName ? i18nc("Install the version of an app that comes from Snap, Flatpak, etc", "Install from %1", backendName) : ""
    }
}
