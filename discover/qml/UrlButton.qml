import QtQuick 2.2
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.1 as QQC2

LinkButton
{
    id: button
    property string url
    text: url
    visible: text.length>0

    acceptedButtons: Qt.LeftButton | Qt.RightButton
    onClicked: {
        if (mouse.button === Qt.RightButton)
            menu.popup()
        else
            Qt.openUrlExternally(url)
    }

    QQC2.Menu {
        id: menu
        QQC2.MenuItem {
            text: i18n("Copy link address")
            onClicked: app.copyTextToClipboard(button.url)
        }
    }
}
