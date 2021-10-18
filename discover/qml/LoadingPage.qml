import org.kde.kirigami 2.14 as Kirigami

Kirigami.Page {
    title: label.text
    readonly property bool isHome: true
    Kirigami.Heading {
        id: label
        text: i18n("Loading…")
        anchors.centerIn: parent
    }
}
