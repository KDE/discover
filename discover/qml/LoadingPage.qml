import org.kde.kirigami as Kirigami

Kirigami.Page {
    readonly property bool isHome: true

    title: placeholder.text

    Kirigami.LoadingPlaceholder {
        id: placeholder
        anchors.centerIn: parent
    }
}
