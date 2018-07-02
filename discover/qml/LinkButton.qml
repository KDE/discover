import QtQuick 2.2
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.1 as QQC2

QQC2.Label {
    id: control

    property QtObject action: null //some older Qt versions don't support the namespacing in Kirigami.Action
    property alias acceptedButtons: area.acceptedButtons
    text: action ? action.text : ""
    enabled: !action || action.enabled
    onClicked: if (action) action.trigger()

    font: control.font
    color: enabled ? Kirigami.Theme.linkColor : Kirigami.Theme.textColor
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter
    elide: Text.ElideRight

    signal clicked(QtObject mouse)
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onContainsMouseChanged: {
            control.font.underline = containsMouse && control.enabled
        }

        onClicked: control.clicked(mouse)
    }
}
