import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami

ApplicationsListPage {
    id: page
    stateFilter: 2

    title: i18n("Installed")
    compact: true

    header: PageHeader {
        width: parent.width
        background: "https://c2.staticflickr.com/8/7146/6783941909_30c7c5d52f_b.jpg"
    }
}
