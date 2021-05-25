import org.kde.kirigami 2.14 as Kirigami

Kirigami.Page {
    title: label.text
    Kirigami.Heading {
        id: label
        text: i18n("Loading…")
        anchors.centerIn: parent
    }
}
