import QtQuick 2.6
import QtQuick.Templates 2.0 as T
import org.kde.kirigami 1.0 as Kirigami

T.ToolButton {
    id: control
    property QtObject menu: null

    implicitWidth: textItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: textItem.implicitHeight + topPadding + bottomPadding
    baselineOffset: contentItem.y + contentItem.baselineOffset

    padding: Kirigami.Units.smallSpacing * 3
    readonly property alias textColor: textItem.color
    hoverEnabled: true

    onClicked: {
        if (menu) {
            menu.popup()
        }
    }

    contentItem: Text {
        id: textItem
        text: control.text
        font: control.font
        color: control.hovered ? Kirigami.Theme.viewBackgroundColor : Kirigami.Theme.linkColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        color: Kirigami.Theme.linkColor
        visible: control.hovered
    }
}
