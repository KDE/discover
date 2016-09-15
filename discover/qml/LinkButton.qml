import QtQuick 2.2
import QtGraphicalEffects 1.0
import org.kde.kirigami 1.0

Text {
    id: control

    property Action action: null
    text: action ? action.text : ""
    enabled: !action || action.enabled
    onClicked: if (action) action.trigger()

    font: control.font
    color: control.shadow ? Theme.highlightedTextColor : Theme.linkColor
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter

    signal clicked()
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onContainsMouseChanged: {
            control.font.underline = containsMouse && control.enabled
        }

        onClicked: control.clicked()
    }
}
