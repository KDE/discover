import QtQuick 2.6
import QtQuick.Templates 2.0 as T
import org.kde.kirigami 1.0 as Kirigami

T.ToolButton {
    id: control

    implicitWidth: textItem.implicitWidth + leftPadding + rightPadding
    implicitHeight: textItem.implicitHeight + topPadding + bottomPadding
    baselineOffset: contentItem.y + contentItem.baselineOffset

    padding: 6
    readonly property alias textColor: textItem.color
    hoverEnabled: true

    onHoveredChanged: textItem.font.underline = control.hovered

    contentItem: Text {
        id: textItem
        text: control.text
        font: control.font
        color: Kirigami.Theme.viewBackgroundColor
        style: Text.Raised
        styleColor: Kirigami.Theme.disabledTextColor
        elide: Text.ElideRight
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }

    background: Item {}
}
