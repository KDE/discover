import QtQuick 2.2
import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.1 as QQC2

LinkButton
{
    property string url
    text: url
    visible: text.length>0
    onClicked: Qt.openUrlExternally(url)
}
