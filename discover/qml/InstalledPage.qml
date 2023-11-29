import QtQuick
import QtQuick.Layouts
import org.kde.discover as Discover
import org.kde.discover.app as DiscoverApp

ApplicationsListPage {
    id: page

    stateFilter: Discover.AbstractResource.Installed
    allBackends: true
    sortProperty: "installedPageSorting"
    defaultSortRole: Discover.ResourcesProxyModel.NameRole

    name: i18n("Installed")
    compact: true
    showRating: false
    showSize: true
    canNavigate: false

    listHeader: null
}
