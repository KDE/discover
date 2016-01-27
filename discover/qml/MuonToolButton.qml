import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.kquickcontrolsaddons 2.0

/**
 * The reason for this shitty component is that it isn't possible to have QtQuick.Controls.ToolButton
 * show both text and icon.
 */

Item
{
    id: root
    property alias text: label.text
    property alias iconName: icon.icon
    property alias tooltip: button.tooltip
    property alias checkable: button.checkable
    property alias checked: button.checked
    property alias exclusiveGroup: button.exclusiveGroup
    property QtObject action

    Layout.minimumWidth: layout.Layout.minimumWidth
    implicitHeight: layout.Layout.preferredHeight

    clip: true
    enabled: action.enabled

    signal clicked()
    onClicked: if (action) action.trigger()

    ToolButton {
        id: button
        anchors.fill: parent
        enabled: root.enabled
        onClicked: { root.clicked() }

        tooltip: root.action ? root.action.tooltip : ""
        action: Action {
            checkable: root.action && root.action.checkable
            checked: root.action && root.action.checked
            onTriggered: {
                checked = Qt.binding(function() { return root.action && root.action.checked})
            }
        }

        RowLayout {
            id: layout
            anchors {
                fill: parent
                margins: 3
            }

            Layout.preferredHeight: 32
            QIconItem {
                id: icon
                Layout.alignment: Qt.AlignVCenter
                anchors.verticalCenter: parent.verticalCenter
                Layout.minimumWidth: layout.Layout.preferredHeight*0.8
                height: Layout.minimumWidth
                icon: root.action ? root.action.iconName : ""
            }
            Label {
                id: label

                function removeAmpersand(text) {
                    return text.replace("&", "");
                }

                text: root.action ? removeAmpersand(root.action.text) : ""
                Layout.fillWidth: true
                Layout.minimumWidth: text == "" ? 0 : (10+label.implicitWidth)
            }
        }
    }
}
