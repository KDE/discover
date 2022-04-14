import org.kde.kirigami 2.19 as Kirigami

Kirigami.Page {
    title: label.text
    readonly property bool isHome: true

    Kirigami.LoadingPlaceholder {
        anchors.centerIn: parent
    }
}
