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

    Layout.minimumHeight: label.font.pixelSize
    Layout.minimumWidth: layout.Layout.minimumWidth
    Layout.preferredHeight: label.font.pixelSize*3

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

            QIconItem {
                id: icon
                anchors.verticalCenter: parent.verticalCenter
                Layout.alignment: Qt.AlignVCenter
                Layout.preferredWidth: label.font.pixelSize*2
                Layout.preferredHeight: label.font.pixelSize*2
                Layout.maximumHeight: button.height
                icon: root.action ? root.action.iconName : ""
            }
            Label {
                id: label

                text: root.action ? root.action.text : ""
                Layout.fillWidth: true
                Layout.minimumWidth: text == "" ? 0 : (10+label.implicitWidth)
            }
        }
    }
}
