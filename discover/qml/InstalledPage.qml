import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 2.0 as Kirigami

ApplicationsListPage {
    id: page
    stateFilter: AbstractResource.Installed
    sortRole: ResourcesProxyModel.RatingCountRole
    sortOrder: Qt.DescendingOrder

    title: i18n("Installed")
    compact: true
    canNavigate: false

    listHeader: PageHeader {
        backgroundImage.source: "qrc:/banners/installedcrop.jpg"
    }
}
