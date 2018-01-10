import QtQuick 2.1
import QtQuick.Controls 2.1
import QtQuick.Layouts 1.1
import org.kde.discover 2.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.1 as Kirigami

ApplicationsListPage {
    id: page
    stateFilter: AbstractResource.Installed
    sortRole: ResourcesProxyModel.NameRole
    sortOrder: Qt.AscendingOrder
    allBackends: true

    title: i18n("Installed")
    compact: true
    canNavigate: false

    listHeader: null
    header: ToolBar {
        Kirigami.Heading {
            anchors {
                right: parent.right
                rightMargin: Kirigami.Units.smallSpacing
            }
            text: page.title
        }
    }
}
