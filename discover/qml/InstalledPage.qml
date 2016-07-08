import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0

ApplicationsListPage {
    id: page
    stateFilter: 2
    preferList: true

    Component.onCompleted: {
        page.changeSorting("size", Qt.DescendingOrder, "")
    }

    readonly property var icon: "applications-other"
    readonly property string title: i18n("Installed")

    header: PageHeader {
        width: parent.width
        RowLayout {
            Item { Layout.fillWidth: true }
            Label {
                visible: TransactionModel.count>0
                text: i18np("%1 job pending...", "%1 jobs pending...", TransactionModel.count)
            }
            Item { Layout.fillWidth: true }
            Label { text: i18n("Sort by ") }
            Button {
                text: page.currentSortAction
                menu: page.sortMenu
            }
        }
    }
}
