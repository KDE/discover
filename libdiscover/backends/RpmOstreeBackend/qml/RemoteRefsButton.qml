import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.kirigami 2.1 as Kirigami

Button
{
    id: root
    text: i18n("Switch to %1", resource.getRecentRemoteRefs())

    onClicked: resource.rebaseToNewVersion()
    visible: resource.isInstalled && resource.isRecentRefsAvaliable()
}

