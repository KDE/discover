import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.muon 1.0

ApplicationsListPage {
    id: page
    stateFilter: 2
    preferList: true
    
    Component.onCompleted: {
        page.changeSorting("canUpgrade", Qt.AscendingOrder, "canUpgrade")
    }

    property var icon: "applications-other"
    property string title: i18n("Installed")
}
