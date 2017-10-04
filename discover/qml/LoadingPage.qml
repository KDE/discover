import org.kde.kirigami 2.0 as Kirigami
import QtQuick.Controls 2.1 as QQC2

Kirigami.Page {
    title: label.text
    QQC2.Label {
        id: label
        text: i18n("Loading...")
        font.pointSize: 52
        anchors.centerIn: parent
    }
}
