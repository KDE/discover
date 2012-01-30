import QtQuick 1.0
import org.kde.plasma.components 0.1

Rectangle {
    height: 400
    width: 300
    color: "lightgrey"
    
    Page {
        id: init
        anchors.margins: 10

        ListView {
            id: pluginsView
            anchors.fill: parent

            delegate:
                ToolButton {
                    iconSource: decoration
                    text: i18n("%1 - %2", title, subtitle)

                    onClicked: goToPage(model.path, decoration)
                }
//             model: CategoryModel {}
        }

        tools: ToolBarLayout {}
    }
    
    PageStack
    {
        id: pageStack
        width: parent.width
        anchors.fill: parent

        initialPage: init
    }
}