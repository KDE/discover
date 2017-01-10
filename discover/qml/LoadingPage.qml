import org.kde.kirigami 2.0 as Kirigami

Kirigami.Page {
    title: label.text
    Kirigami.Label {
        id: label
        text: i18n("Loading...")
        font.pointSize: 52
        anchors.centerIn: parent
    }
}
