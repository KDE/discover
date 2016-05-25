import QtQuick 2.1
import QtQuick.Layouts 1.1
import QtQuick.Controls 1.1
import org.kde.discover 1.0
import org.kde.discover.app 1.0
import org.kde.kirigami 1.0 as Kirigami
import "navigation.js" as Navigation

Kirigami.ApplicationWindow
{
    id: window
    readonly property Component applicationListComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    readonly property Component applicationComp: Qt.createComponent("qrc:/qml/ApplicationPage.qml")
    readonly property Component categoryComp: Qt.createComponent("qrc:/qml/ApplicationsListPage.qml")
    readonly property Component reviewsComp: Qt.createComponent("qrc:/qml/ReviewsPage.qml")

    //toplevels
    readonly property Component topBrowsingComp: Qt.createComponent("qrc:/qml/BrowsingPage.qml")
    readonly property Component topInstalledComp: Qt.createComponent("qrc:/qml/InstalledPage.qml")
    readonly property Component topUpdateComp: Qt.createComponent("qrc:/qml/UpdatesPage.qml")
    readonly property Component topSourcesComp: Qt.createComponent("qrc:/qml/SourcesPage.qml")
    readonly property QtObject stack: window.pageStack
    property Component currentTopLevel: defaultStartup ? topBrowsingComp : loadingComponent
    property bool defaultStartup: true
    property bool navigationEnabled: true

    objectName: "DiscoverMainWindow"

    visible: true

    Component.onCompleted: {
        Helpers.mainWindow = window
        if (app.isRoot)
            showPassiveNotification(i18n("Running as <em>root</em> is discouraged and unnecessary."));
    }

    function clearSearch() {
        if (loader.item)
            loader.item.clearSearch();
    }

    Component {
        id: loadingComponent
        Item {
            readonly property string title: ""
            readonly property string icon: "applications-other"
            Label {
                id: label
                text: i18n("Loading...")
                font.pointSize: 52
                anchors.centerIn: parent
            }
        }
    }

    ExclusiveGroup { id: appTabs }

    property list<Action> awesome: [
        TopLevelPageData {
            iconName: "tools-wizard"
            text: i18n("Discover")
            component: topBrowsingComp
            objectName: "discover"
            shortcut: "Alt+D"
        },
        TopLevelPageData {
            iconName: "installed"
            text: TransactionModel.count == 0 ? i18n("Installed") : i18n("Installing...")
            component: topInstalledComp
            objectName: "installed"
            shortcut: "Alt+I"
        },
        TopLevelPageData {
            iconName: enabled ? "update-low" : "update-none"
            text: !enabled ? i18n("No Updates") : i18nc("Update section name", "Update (%1)", ResourcesModel.updatesCount)
            enabled: ResourcesModel.updatesCount>0
            component: topUpdateComp
            objectName: "update"
            shortcut: "Alt+U"
        }
    ]

    Connections {
        target: app
        onOpenApplicationInternal: {
            currentTopLevel = topBrowsingComp;
            Navigation.openApplication(app)
        }
        onListMimeInternal:  {
            currentTopLevel = topBrowsingComp;
            Navigation.openApplicationMime(mime)
        }
        onListCategoryInternal:  {
            currentTopLevel = topBrowsingComp;
            Navigation.openCategory(cat)
        }
    }

    Menu {
        id: moreMenu
        MenuItem {
            action: TopLevelPageData {
                text: i18n("Configure Sources...")
                iconName: "repository"
                shortcut: "Alt+S"
                component: topSourcesComp
            }
        }
        MenuSeparator {}
        Instantiator {
            id: actionsInstantiator
            model: MessageActionsModel {}
            delegate: MenuItem { action: ActionBridge { action: model.action } }

            onObjectAdded: moreMenu.insertItem(index, object)
            onObjectRemoved: moreMenu.removeItem(object)
        }
        MenuSeparator {
            visible: actionsInstantiator.count > 0
        }
        MenuItem { action: ActionBridge { action: app.action("options_configure_keybinding"); } }
        MenuItem { action: ActionBridge { action: app.action("help_about_app"); } }
        MenuItem { action: ActionBridge { action: app.action("help_report_bug"); } }
    }

    globalDrawer: DiscoverWindow_PlasmaPhone {}

    onCurrentTopLevelChanged: {
        if(currentTopLevel.status==Component.Error) {
            console.log("status error: "+currentTopLevel.errorString())
        }
        var stackView = window.pageStack;
        stackView.clear()
        stackView.push(currentTopLevel, {}, window.status!=Component.Ready)
    }

    Connections {
        target: app
        onPreventedClose: showPassiveNotification(i18n("Could not close the application, there are tasks that need to be done."), 3000)
        onUnableToFind: showPassiveNotification(i18n("Unable to find resource: %1", resid));
    }

//     ColumnLayout {
//         spacing: 0
//         anchors.fill: parent
//
//         Repeater {
//             model: MessageActionsModel {
//                 filterPriority: QAction.HighPriority
//             }
//             delegate: MessageAction {
//                 Layout.fillWidth: true
//                 height: Layout.minimumHeight
//                 theAction: action
//             }
//         }
//     }
}
