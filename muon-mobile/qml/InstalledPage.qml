import QtQuick 1.1
import org.kde.plasma.components 0.1
import "navigation.js" as Navigation

ApplicationsListPage {
    stateFilter: (1<<8)
    sortRole: "name"
    sortOrder: 0
}