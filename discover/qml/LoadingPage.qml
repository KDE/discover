import org.kde.kirigami 2.19 as Kirigami

Kirigami.Page {
    title: placeholder.text
    readonly property bool isHome: true

    Kirigami.LoadingPlaceholder {
        id: placeholder
        anchors.centerIn: parent
    }
}
